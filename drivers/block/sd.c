/* JH7110 SDIO1 / DW-MSHC。寄存器和 DMA 契约见 docs/visionfive2-sd.md。 */
#include <drivers/sd.h>
#include <kernel/arch.h>
#include <kernel/print.h>
#include <uapi/disk.h>
#include <platform.h>
#if BLOCK_IS_SD
#define ERROR 0xbfc2U
#define START (1U << 31)
#define HOLD (1U << 29)
#define RESP ((1U << 6) | (1U << 8))
static struct __attribute__((aligned(64))) { uint32 word[16]; } descriptor;
static uint64 data_address;
static uint32 seen, dma_seen;
static bool wide, active;
static unsigned ids(void) { return wide ? 0x90 : 0x8c; }
static unsigned idi(void) { return wide ? 0x94 : 0x90; }
static uint32 rd(unsigned off) { return *(volatile uint32 *)(BLOCK_BASE + off); }
static void wr(unsigned off, uint32 v) { *(volatile uint32 *)(BLOCK_BASE + off) = v; }
/* CCACHE FLUSH64 清理并失效一致性域中的 64 字节 cache line。
 * 使用物理地址，fence 仅排序；页和描述符独占完整 cache line。 */
static void sync_dma(uint64 pa, uint64 size)
{
    arch_dma_fence();
    for (uint64 p = pa; p < pa + size; p += 64) {
        *(volatile uint64 *)(CCACHE_BASE + 0x200) = p;
        arch_dma_fence();
    }
}
static int wait_clear(unsigned off, uint32 mask)
{
    for (unsigned i = 0; i < 10000000; ++i) if (!(rd(off) & mask)) return 0;
    return -1;
}
static int command(unsigned cmd, uint32 arg, uint32 flags)
{
    wr(0x44, ~0U); wr(0x28, arg); arch_dma_fence();
    wr(0x2c, START | HOLD | flags | cmd);
    for (unsigned i = 0; i < 10000000; ++i) {
        uint32 status = rd(0x44);
        if (status & ERROR) return -1;
        if (status & 4) { wr(0x44, status); return 0; }
    }
    return -1;
}
static int clock_update(uint32 divider)
{
    wr(0x10, 0); wr(0x08, divider); wr(0x0c, 0);
    wr(0x2c, START | HOLD | (1U << 21) | (1U << 13));
    if (wait_clear(0x2c, START)) return -1;
    wr(0x10, 1); wr(0x2c, START | HOLD | (1U << 21) | (1U << 13));
    return wait_clear(0x2c, START);
}
int sd_init(void)
{
    /* U-Boot 已配置 3.3V、电源、引脚、复位/时钟，ciu <= 200MHz。
     * 内核重新枚举 SDHC/SDXC，并独占 IDMAC；不继承 U-Boot 请求或描述符。 */
    wr(0x24, 0); wr(0x00, 7);
    if (wait_clear(0, 7)) return -1;
    wide = !!(rd(0x70) & (1U << 27));
    wr(idi(), 0); wr(0x80, 1);
    if (wait_clear(0x80, 1)) return -1;
    wr(0x04, 1); wr(0x18, 0); wr(0x74, 0); wr(0x14, ~0U);
    if (clock_update(255) || command(0, 0, 1U << 15) || command(8, 0x1aa, RESP)) return -1;
    if ((rd(0x30) & 0xfff) != 0x1aa) return -1;
    uint32 ocr = 0;
    for (unsigned i = 0; i < 10000; ++i) {
        if (command(55, 0, RESP) || command(41, 0x40300000, 1U << 6)) return -1;
        ocr = rd(0x30); if (ocr & START) break;
    }
    if ((ocr & 0xc0000000) != 0xc0000000) return -1;
    if (command(2, 0, RESP | (1U << 7)) || command(3, 0, RESP)) return -1;
    uint32 rca = rd(0x30) & 0xffff0000;
    if (!rca || command(9, rca, RESP | (1U << 7))) return -1;
    if ((rd(0x3c) >> 30) != 1) return -1; /* CSD v2 */
    uint64 sectors = (1ULL + ((rd(0x34) >> 16) | ((rd(0x38) & 0x3f) << 16))) * 1024;
    if (sectors < SD_FIRST_SECTOR + SD_SECTORS) return -1;
    if (command(7, rca, RESP) || wait_clear(0x48, 1U << 9) || clock_update(4)) return -1;
    wr(0x4c, (2U << 28) | (15U << 16) | 16); /* FIFO depth=32 */
    wr(ids(), ~0U); wr(0x44, ~0U);
    wr(0x00, (1U << 25) | (1U << 5) | (1U << 4));
    active = false; return 0;
}
int sd_submit(uint64 pa, uint32 block, bool write)
{
    if (active || block >= TOTAL_BLOCKS || (uint64)block * 8 + 8 > SD_SECTORS || pa % 4096) return -1;
    if (wait_clear(0x48, 1U << 9)) return -1;
    wr(0x24, 0); wr(idi(), 0); wr(0x00, rd(0) | 6);
    if (wait_clear(0, 6)) return -1;
    wr(0x80, 1); if (wait_clear(0x80, 1)) return -1;
    for (int i = 0; i < 16; ++i) descriptor.word[i] = 0;
    descriptor.word[0] = START | 8 | 4; /* OWN, first, last */
    descriptor.word[wide ? 2 : 1] = 4096;
    descriptor.word[wide ? 4 : 2] = pa;
    if (wide) descriptor.word[5] = pa >> 32;
    data_address = pa;
    sync_dma(pa, 4096); sync_dma((uint64)&descriptor, sizeof(descriptor));
    wr(0x88, (uint64)&descriptor); if (wide) wr(0x8c, (uint64)&descriptor >> 32);
    wr(ids(), ~0U); wr(0x44, ~0U); seen = dma_seen = 0;
    wr(0x1c, 512); wr(0x20, 4096);
    wr(0x80, (1U << 7) | 2); wr(idi(), 0x337); wr(0x24, ERROR | 4 | 8 | (1U << 14));
    active = true;
    wr(0x28, SD_FIRST_SECTOR + (uint64)block * 8); arch_dma_fence();
    wr(0x2c, START | HOLD | RESP | (1U << 9) | (1U << 12) | (1U << 13) | (write ? (1U << 10) | 25 : 18));
    return 0;
}
int sd_complete(int *result)
{
    uint32 status = rd(0x44), dma = rd(ids());
    wr(0x44, status); wr(ids(), dma);
    if (!active) return 0;
    seen |= status; dma_seen |= dma;
    /* 不确定 DMA 是否停止时不可交还页面；沿用内核简化致命失败契约。 */
    if ((seen & ERROR) || (dma_seen & 0x234)) panic("SDIO transfer error; DMA memory retained");
    if ((seen & (4 | 8 | (1U << 14))) != (4 | 8 | (1U << 14)) || !(dma_seen & 3)) return 0;
    if ((rd(0x30) & 0xfdffe008U)) panic("SD card R1 error");
    if (wait_clear(0x48, 1U << 9)) panic("SD card busy; DMA memory retained");
    wr(0x24, 0); wr(idi(), 0); wr(0x80, 0);
    sync_dma((uint64)&descriptor, sizeof(descriptor));
    if (*(volatile uint32 *)&descriptor.word[0] & (START | (1U << 30))) panic("SD descriptor ownership");
    sync_dma(data_address, 4096);
    active = false; *result = 0; return 1;
}
#endif
