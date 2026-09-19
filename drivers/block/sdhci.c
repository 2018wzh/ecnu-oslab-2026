/* SD 卡驱动 (Synopsys DesignWare MSHC / SDHCI 兼容), VisionFive2 平台用轮询 + PIO。 */

#include <kernel/types.h>
#include <kernel/print.h>
#include <kernel/block.h>
#include <platform.h>

/* ---- 寄存器偏移 ---- */
#define SDHCI_SDMA_ADDR        0x000
#define SDHCI_BLOCK_SIZE       0x004
#define SDHCI_BLOCK_COUNT      0x006
#define SDHCI_ARGUMENT         0x008
#define SDHCI_TRANSFER_MODE    0x00C
#define SDHCI_COMMAND          0x00E
#define SDHCI_RESPONSE         0x010   /* 4 个 32 位寄存器 */
#define SDHCI_BUFFER_DATA      0x020
#define SDHCI_PRESENT_STATE    0x024
#define SDHCI_HOST_CONTROL     0x028
#define SDHCI_POWER_CONTROL    0x029
#define SDHCI_CLOCK_CONTROL    0x02C
#define SDHCI_TIMEOUT_CONTROL  0x02E
#define SDHCI_SOFTWARE_RESET   0x02F
#define SDHCI_NORMAL_INT_STATUS 0x030
#define SDHCI_ERROR_INT_STATUS 0x032
#define SDHCI_NORMAL_INT_STATUS_EN 0x034
#define SDHCI_ERROR_INT_STATUS_EN 0x036
#define SDHCI_CAPABILITIES     0x040
#define SDHCI_CLOCK_CAPABILITIES 0x044
#define SDHCI_HOST_VERSION     0x0FE

/* ---- PRESENT_STATE 的位 ---- */
#define PS_CMD_INHIBIT         (1u << 0)    /* 命令线忙 */
#define PS_DAT_INHIBIT         (1u << 1)    /* 数据线忙 */
#define PS_BUFFER_WRITE_READY  (1u << 10)   /* 可以往数据端口写 */
#define PS_BUFFER_READ_READY   (1u << 11)   /* 可以从数据端口读 */
#define PS_CARD_INSERTED       (1u << 16)

/* ---- 中断状态位 (用到的几个) ---- */
#define INT_CMD_COMPLETE       (1u << 0)
#define INT_TRANSFER_COMPLETE  (1u << 1)
#define INT_BUFFER_READ_READY  (1u << 5)
#define INT_BUFFER_WRITE_READY (1u << 4)
#define INT_ERROR              (1u << 15)

/* ---- CLOCK_CONTROL ---- */
#define CLK_INTERNAL_EN        (1u << 0)
#define CLK_SD_EN              (1u << 2)
#define CLK_DIV_SHIFT          8
#define CLK_DIV_MASK           (0xFFu << CLK_DIV_SHIFT)

/* ---- 命令寄存器: 响应类型 ---- */
#define CMD_RESP_NONE          0u
#define CMD_RESP_136           (1u << 0)    /* R2: 136 位 */
#define CMD_RESP_48            (2u << 0)    /* R1/R5/R6/R7: 48 位 */
#define CMD_RESP_48_BUSY       (3u << 0)
#define CMD_CRC_CHECK          (1u << 3)
#define CMD_INDEX_CHECK        (1u << 4)
#define CMD_DATA_PRESENT       (1u << 5)

/* ---- SD 命令号 ---- */
#define SD_GO_IDLE_STATE       0
#define SD_SEND_IF_COND        8
#define SD_ALL_SEND_CID        2
#define SD_SEND_RELATIVE_ADDR  3
#define SD_SEND_CSD            9
#define SD_SELECT_CARD         7
#define SD_SET_BLOCKLEN        16
#define SD_READ_SINGLE_BLOCK   17
#define SD_WRITE_BLOCK         24
#define SD_APP_CMD             55
#define SD_APP_OP_COND         41

/* ---- 传输模式位 ---- */
#define TM_DATA_READ           (1u << 4)    /* 1 = 卡 -> 主机 */
#define TM_BLOCK_COUNT_EN      (1u << 1)

#define REG8(off)  (*(volatile uint8  *)(PLAT_SDIO0_BASE + (off)))
#define REG16(off) (*(volatile uint16 *)(PLAT_SDIO0_BASE + (off)))
#define REG32(off) (*(volatile uint32 *)(PLAT_SDIO0_BASE + (off)))

// 轮询超时上限: 取很大, 避免等待无上限时硬件出错表现为卡死。
#define SDHCI_SPIN_LIMIT   10000000u

// 驱动状态: 本课程只有一个控制器, 用文件级静态量而不是 priv 指针。
static uint64 g_card_blocks;      /* 卡容量, 单位 512 字节块 */
static uint32 g_rca;              /* 相对卡地址 (CMD3 返回) */
static uint32 g_high_capacity;    /* 1 = SDHC/SDXC (按块寻址) */

// 等待命令线空闲, 超时返回 -1。
static int wait_cmd_idle(void)
{
	for (uint32 i = 0; i < SDHCI_SPIN_LIMIT; i++) {
		if (!(REG32(SDHCI_PRESENT_STATE) & PS_CMD_INHIBIT))
			return 0;
	}
	return -1;
}

// 等待数据线空闲, 超时返回 -1。
static int wait_dat_idle(void)
{
	for (uint32 i = 0; i < SDHCI_SPIN_LIMIT; i++) {
		if (!(REG32(SDHCI_PRESENT_STATE) & PS_DAT_INHIBIT))
			return 0;
	}
	return -1;
}

// 发一条命令并等它完成: 命令寄存器是触发而非状态, 必须等 CMD_COMPLETE
// 才能读响应, 出错位须清掉以免残留错误影响下一条命令。
static int sd_cmd(uint32 idx, uint32 arg, uint32 resp_type, uint32 *resp)
{
	if (wait_cmd_idle() != 0)
		return -1;
	if (wait_dat_idle() != 0)
		return -1;

	/* 清中断状态 (写 1 清除) */
	REG32(SDHCI_NORMAL_INT_STATUS) = 0xFFFFFFFFu;
	REG32(SDHCI_ERROR_INT_STATUS)  = 0xFFFFFFFFu;

	REG32(SDHCI_ARGUMENT) = arg;
	REG16(SDHCI_COMMAND)  = (uint16)((idx << 8) | resp_type | CMD_CRC_CHECK | CMD_INDEX_CHECK);

	/* 等命令完成 */
	for (uint32 i = 0; i < SDHCI_SPIN_LIMIT; i++) {
		uint32 st = REG32(SDHCI_NORMAL_INT_STATUS);
		if (st & INT_ERROR)
			return -1;
		if (st & INT_CMD_COMPLETE)
			break;
		if (i == SDHCI_SPIN_LIMIT - 1)
			return -1;
	}

	if (resp) {
		resp[0] = REG32(SDHCI_RESPONSE + 0);
		resp[1] = REG32(SDHCI_RESPONSE + 4);
		resp[2] = REG32(SDHCI_RESPONSE + 8);
		resp[3] = REG32(SDHCI_RESPONSE + 12);
	}
	return 0;
}

// 设置 SD 时钟: 分频 = 基频 / (2*divider), 基频从 CAPABILITIES 读;
// 改分频前必须先关 SD_CLOCK_EN, 改完再打开。
static int sd_set_clock(uint32 target_hz)
{
	uint32 caps = REG32(SDHCI_CAPABILITIES);
	uint32 base_mhz = (caps >> 8) & 0xFF;
	uint32 base_hz = base_mhz * 1000000u;
	if (base_hz == 0)
		return -1;

	uint32 div = 1;
	while (div < 256 && (base_hz / (2 * div)) > target_hz)
		div++;
	div = div / 2;              /* 寄存器的分频值是"除数/2" */
	if (div > 0xFF)
		div = 0xFF;

	REG16(SDHCI_CLOCK_CONTROL) = 0;
	REG16(SDHCI_CLOCK_CONTROL) = (uint16)((div << CLK_DIV_SHIFT) | CLK_INTERNAL_EN);

	/* 等内部时钟稳定 (规范要求, 有些实现不置这个位, 所以给超时) */
	for (uint32 i = 0; i < 100000; i++) {
		if (REG16(SDHCI_CLOCK_CONTROL) & CLK_INTERNAL_EN)
			break;
	}

	REG16(SDHCI_CLOCK_CONTROL) |= CLK_SD_EN;
	return 0;
}

// 单块读: CMD17 + PIO。
static int sd_read_block(uint64 block, void *buf)
{
	if (wait_dat_idle() != 0)
		return -1;

	REG32(SDHCI_NORMAL_INT_STATUS) = 0xFFFFFFFFu;
	REG32(SDHCI_ERROR_INT_STATUS)  = 0xFFFFFFFFu;

	REG16(SDHCI_BLOCK_SIZE)  = 512;     /* 低 12 位是大小, 高位是 SDMA 边界 */
	REG16(SDHCI_BLOCK_COUNT) = 1;
	REG16(SDHCI_TRANSFER_MODE) = TM_DATA_READ;

	/* SDHC/SDXC 按**块号**寻址; SDSC 按**字节地址**寻址。
	 * 混淆的症状是"读出来的数据和期望的差了 512 倍的位置"。 */
	uint32 arg = g_high_capacity ? (uint32)block : (uint32)(block * 512);

	if (sd_cmd(SD_READ_SINGLE_BLOCK, arg, CMD_RESP_48 | CMD_DATA_PRESENT, NULL) != 0)
		return -1;

	/* PIO: 等"缓冲区可读", 然后从数据端口搬 512 字节 */
	uint32 *p = (uint32 *)buf;
	for (uint32 i = 0; i < 512 / 4; i++) {
		uint32 ok = 0;
		for (uint32 s = 0; s < SDHCI_SPIN_LIMIT; s++) {
			if (REG32(SDHCI_PRESENT_STATE) & PS_BUFFER_READ_READY) {
				ok = 1;
				break;
			}
			if (REG32(SDHCI_ERROR_INT_STATUS) != 0)
				return -1;
		}
		if (!ok)
			return -1;
		p[i] = REG32(SDHCI_BUFFER_DATA);
	}

	/* 等传输完成 */
	for (uint32 i = 0; i < SDHCI_SPIN_LIMIT; i++) {
		if (REG32(SDHCI_NORMAL_INT_STATUS) & INT_TRANSFER_COMPLETE)
			break;
		if (REG32(SDHCI_ERROR_INT_STATUS) != 0)
			return -1;
		if (i == SDHCI_SPIN_LIMIT - 1)
			return -1;
	}
	return 0;
}

// 单块写: CMD24 + PIO。
static int sd_write_block(uint64 block, const void *buf)
{
	if (wait_dat_idle() != 0)
		return -1;

	REG32(SDHCI_NORMAL_INT_STATUS) = 0xFFFFFFFFu;
	REG32(SDHCI_ERROR_INT_STATUS)  = 0xFFFFFFFFu;

	REG16(SDHCI_BLOCK_SIZE)  = 512;
	REG16(SDHCI_BLOCK_COUNT) = 1;
	REG16(SDHCI_TRANSFER_MODE) = 0;     /* 0 = 主机 -> 卡 */

	uint32 arg = g_high_capacity ? (uint32)block : (uint32)(block * 512);

	if (sd_cmd(SD_WRITE_BLOCK, arg, CMD_RESP_48 | CMD_DATA_PRESENT, NULL) != 0)
		return -1;

	const uint32 *p = (const uint32 *)buf;
	for (uint32 i = 0; i < 512 / 4; i++) {
		uint32 ok = 0;
		for (uint32 s = 0; s < SDHCI_SPIN_LIMIT; s++) {
			if (REG32(SDHCI_PRESENT_STATE) & PS_BUFFER_WRITE_READY) {
				ok = 1;
				break;
			}
			if (REG32(SDHCI_ERROR_INT_STATUS) != 0)
				return -1;
		}
		if (!ok)
			return -1;
		REG32(SDHCI_BUFFER_DATA) = p[i];
	}

	for (uint32 i = 0; i < SDHCI_SPIN_LIMIT; i++) {
		if (REG32(SDHCI_NORMAL_INT_STATUS) & INT_TRANSFER_COMPLETE)
			break;
		if (REG32(SDHCI_ERROR_INT_STATUS) != 0)
			return -1;
		if (i == SDHCI_SPIN_LIMIT - 1)
			return -1;
	}
	return 0;
}

/* ---- block_ops 接口 ---- */

static int sdhci_read(struct block_device *dev, uint64 blockno,
                      void *buf, uint32 nblocks)
{
	(void)dev;
	if (blockno + nblocks > g_card_blocks)
		return -1;

	uint8 *p = (uint8 *)buf;
	// 逐块发 CMD17 单块读, 避免 CMD18 多块读需 CMD12 收尾的陷阱。
	for (uint32 i = 0; i < nblocks; i++) {
		if (sd_read_block(blockno + i, p + (uint64)i * 512) != 0)
			return -1;
	}
	return 0;
}

static int sdhci_write(struct block_device *dev, uint64 blockno,
                       const void *buf, uint32 nblocks)
{
	(void)dev;
	if (blockno + nblocks > g_card_blocks)
		return -1;

	const uint8 *p = (const uint8 *)buf;
	for (uint32 i = 0; i < nblocks; i++) {
		if (sd_write_block(blockno + i, p + (uint64)i * 512) != 0)
			return -1;
	}
	return 0;
}

static struct block_ops sdhci_ops = {
	.read  = sdhci_read,
	.write = sdhci_write,
};

static struct block_device sdhci_dev = {
	.name    = "sdhci",
	.ops     = &sdhci_ops,
	.priv    = NULL,
	.nblocks = 0,
};

// 从 CSD 计算容量: SDSC 与 SDHC/SDXC 两套算法完全不同, 留意别写错。
static uint64 csd_capacity_blocks(const uint32 *csd)
{
	uint32 csd_structure = (csd[0] >> 30) & 0x3;

	if (csd_structure == 1) {
		/* C_SIZE 在 csd[1] 的 bit 5..0 和 csd[2] 的 bit 31..16 */
		uint32 c_size = ((csd[1] & 0x3F) << 16) | ((csd[2] >> 16) & 0xFFFF);
		/* (C_SIZE + 1) * 512 KB = (C_SIZE+1) * 1024 个 512 字节块 */
		return ((uint64)c_size + 1) * 1024;
	}

	/* SDSC: C_SIZE 在 csd[1] 的 bit 11..0 与 csd[2] 的 bit 31..22 */
	uint32 c_size  = ((csd[1] & 0x3FF) << 2) | ((csd[2] >> 30) & 0x3);
	uint32 c_mult  = (csd[2] >> 15) & 0x7;
	uint32 read_bl = csd[2] & 0xF;
	uint64 bytes = ((uint64)c_size + 1)
	             << (c_mult + 2 + read_bl);
	return bytes / 512;
}

// 初始化 SD 卡并注册块设备: 按 SD 规范的命令序列复位、识别、加电、设时钟。
void block_init(void)
{
	uint32 resp[4];

	printf("[block] 初始化 SD 卡控制器 (DW MSHC) @ 0x%lx\n",
	       (uint64)PLAT_SDIO0_BASE);

	/* ---- 1. 软件复位 (整个主机控制器) ---- */
	REG8(SDHCI_SOFTWARE_RESET) = 0x01;
	for (uint32 i = 0; i < SDHCI_SPIN_LIMIT; i++) {
		if (!(REG8(SDHCI_SOFTWARE_RESET) & 0x01))
			break;
		if (i == SDHCI_SPIN_LIMIT - 1) {
			printf("[block] SD 控制器复位超时\n");
			return;
		}
	}

	/* ---- 2. 上电 (SD 总线电压 3.3V = 0x0E) ---- */
	REG8(SDHCI_POWER_CONTROL) = 0x00;
	REG8(SDHCI_POWER_CONTROL) = 0x0E;
	// 规范要求上电后等至少 1ms 让电压稳定, 这里用一段空转代替精确延时。
	for (volatile uint32 i = 0; i < 100000; i++)
		;

	/* ---- 3. 超时控制: 最大数据超时, 避免慢卡被误判为失败 ---- */
	REG8(SDHCI_TIMEOUT_CONTROL) = 0x0E;

	/* ---- 4. 时钟: 识别阶段用 400 kHz (规范规定的下限) ---- */
	if (sd_set_clock(400000) != 0) {
		printf("[block] 无法设置 SD 时钟 (能力寄存器基频为 0)\n");
		return;
	}

	/* ---- 5. 卡识别 ---- */

	// CMD0 复位不需要 CRC 与响应, 但 sd_cmd 统一加了, 卡会忽略, 无需特判。
	(void)sd_cmd(SD_GO_IDLE_STATE, 0, CMD_RESP_NONE, NULL);

	// CMD8 检查电压范围: 返回 R7 里的 echo 应与我们发出去的 0x1AA 一致, 否则是 SD v1 卡。
	g_high_capacity = 0;
	int is_v2 = 0;
	if (sd_cmd(SD_SEND_IF_COND, 0x1AA, CMD_RESP_48, resp) == 0) {
		if ((resp[0] & 0xFFF) == 0x1AA)
			is_v2 = 1;
	}

	// CMD55+ACMD41 反复问"初始化好了吗": 卡上电后需时间做内部初始化,
	// 对 v2 卡带上 HCS 位 (bit 30) 表示支持高容量卡。
	uint32 acmd41_arg = 0x40000000u;    /* HCS */
	if (!is_v2)
		acmd41_arg = 0;

	int ready = 0;
	for (uint32 i = 0; i < 1000; i++) {
		if (sd_cmd(SD_APP_CMD, 0, CMD_RESP_48, NULL) != 0)
			break;
		if (sd_cmd(SD_APP_OP_COND, acmd41_arg, CMD_RESP_48, resp) != 0)
			break;
		if (resp[0] & 0x80000000u) {    /* busy 位清 0 = 就绪 */
			ready = 1;
			break;
		}
	}
	if (!ready) {
		printf("[block] SD 卡未就绪 (没有插卡? 或卡不响应 ACMD41)\n");
		return;
	}
	/* 卡回应的 CCS 位 (bit 30) 说明它是高容量卡 */
	if (resp[0] & 0x40000000u)
		g_high_capacity = 1;

	/* CMD2: 取 CID (我们不解析它, 但规范要求这一步) */
	if (sd_cmd(SD_ALL_SEND_CID, 0, CMD_RESP_136, resp) != 0) {
		printf("[block] CMD2 (取 CID) 失败\n");
		return;
	}

	/* CMD3: 卡返回自己的 RCA, 之后的命令都用它寻址 */
	if (sd_cmd(SD_SEND_RELATIVE_ADDR, 0, CMD_RESP_48, resp) != 0) {
		printf("[block] CMD3 (取 RCA) 失败\n");
		return;
	}
	g_rca = resp[0] & 0xFFFF0000u;

	/* CMD9: 取 CSD —— 容量在里面 */
	if (sd_cmd(SD_SEND_CSD, g_rca, CMD_RESP_136, resp) != 0) {
		printf("[block] CMD9 (取 CSD) 失败\n");
		return;
	}
	g_card_blocks = csd_capacity_blocks(resp);
	if (g_card_blocks == 0) {
		printf("[block] CSD 解析出 0 块容量 (CSD 结构不认识?)\n");
		return;
	}

	/* CMD7: 选中这张卡, 之后它才会响应数据读写命令 */
	if (sd_cmd(SD_SELECT_CARD, g_rca, CMD_RESP_48, resp) != 0) {
		printf("[block] CMD7 (选中卡) 失败\n");
		return;
	}

	/* CMD16: 设块长 512。高容量卡的块长固定是 512, 但这条命令
	 * 对标准容量卡是必需的 —— 而且成本很低, 所以统一发。 */
	if (sd_cmd(SD_SET_BLOCKLEN, 512, CMD_RESP_48, resp) != 0) {
		printf("[block] CMD16 (设块长) 失败\n");
		return;
	}

	/* ---- 6. 切到高速时钟, 然后注册设备 ---- */
	if (sd_set_clock(25000000) != 0) {
		printf("[block] 切换到 25 MHz 失败\n");
		return;
	}

	sdhci_dev.nblocks = g_card_blocks;
	block_register(&sdhci_dev);

	printf("[block] SD 卡就绪: %s, %lu 块\n",
	       g_high_capacity ? "SDHC/SDXC" : "SDSC",
	       (uint64)g_card_blocks);
}

// 本平台不使用 VirtIO, 提供空实现以保证 generic 的 trap 分发代码可引用该符号。
void virtio_disk_intr(void)
{
}
