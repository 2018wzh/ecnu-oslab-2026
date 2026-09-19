// VisionFive2 (JH7110) 板级实现。
// 与 platform/qemu-virt-riscv64/platform.c 结构完全一样, 只内容不同 (设备区域、
// 中断号)。kernel/ 下的代码看不到这些差异, 只调 platform_map_devices()/
// platform_dispatch_irq() —— 加第三块板子只需再写一个这样的文件。
#include <kernel/types.h>
#include <kernel/print.h>
#include <kernel/mm.h>
#include <kernel/arch.h>
#include <kernel/platform.h>
#include <platform.h>

extern char _image_start[];          /* 本目录的板级描述 (PLAT_* 宏) */

void platform_init(void)
{
	printf("[platform] %s  DRAM  : 0x%lx + %lu MB  kernel: 0x%lx (镜像实际: 0x%lx)\n",
	       PLAT_NAME,
	       (uint64)PLAT_DRAM_BASE, (uint64)(PLAT_DRAM_SIZE / (1024 * 1024)),
	       (uint64)PLAT_KERNEL_BASE, (uint64)_image_start);
	printf("[platform] cpus  : %d (启动核 = %lu, 平台配置 = %d)  uart  : 0x%lx (irq %d)\n",
	       PLAT_NCPU, (uint64)arch_cpu_cold_boot_hart(), PLAT_BOOT_HART,
	       (uint64)PLAT_UART0_BASE, PLAT_UART0_IRQ);
	/* 本平台特有的两个事实: 可用的 hart 区间 (hart 0 是监控核),
	 * 以及 SD 控制器的基地址。 */
	printf("[platform] hart  : %d..%d (hart 0 为监控核)  sdcard: 0x%lx  uart 时钟 %lu Hz\n",
	       PLAT_HART_MIN, PLAT_HART_MAX, (uint64)PLAT_SDIO0_BASE,
	       (uint64)PLAT_UART0_CLOCK);
}

extern void _entry(void);
uint64 platform_secondary_entry(void)
{
	return (uint64)_entry;
}

/* --------------------------------------------------------------------------
 * 映射本平台的设备 MMIO 区域
 *
 * VisionFive2 需要映射:
 *   1. PLIC + UART  (与 QEMU 同类型设备, 但基地址与中断号不同)
 *   2. SD 卡控制器 (DW MSHC / SDHCI 兼容) —— 取代 QEMU 的 VirtIO
 * -------------------------------------------------------------------------- */
int platform_map_devices(uint64 pgtbl)
{
	uint64 a_begin = ALIGN_DOWN(PLAT_PLIC_BASE, PGSIZE);
	uint64 a_end   = ALIGN_UP(PLAT_UART0_BASE + PGSIZE, PGSIZE);
	if (kvm_map(pgtbl, a_begin, a_begin, a_end - a_begin, PERM_R | PERM_W) < 0)
		return -1;

	/* SD 卡控制器: 寄存器区比较大 (SDHCI 规范里偏移可到 0x100 以内,
	 * 但控制器还可能有 vendor 特有的寄存器), 这里映射 4 页足够。 */
	uint64 sd_begin = ALIGN_DOWN(PLAT_SDIO0_BASE, PGSIZE);
	uint64 sd_size  = PGSIZE * 4;
	if (kvm_map(pgtbl, sd_begin, sd_begin, sd_size, PERM_R | PERM_W) < 0)
		return -1;

	printf("[platform] 已映射设备区 [0x%lx, 0x%lx) 与 SD 控制器 [0x%lx, +0x%lx)\n",
	       a_begin, a_end, sd_begin, sd_size);
	return 0;
}

/* --------------------------------------------------------------------------
 * 把外部中断号分发给对应设备
 *
 * VisionFive2 上 UART 中断号是 32 (QEMU 是 10)。
 * 这个差异只在这里体现, kernel/ 完全不知道。
 * -------------------------------------------------------------------------- */
void uart_intr(void);

int platform_dispatch_irq(int irq)
{
	if (irq == PLAT_UART0_IRQ) {
		uart_intr();
		return 1;
	}
	/* SD 卡在本课程里用轮询模式驱动, 不使用中断 (见 platform.h 的
	 * PLAT_SDIO0_IRQ 说明)。真实产品里会用中断来提高效率。 */
	return 0;
}
