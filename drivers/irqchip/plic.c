/* SiFive PLIC (平台级中断控制器) 驱动: QEMU 与 VisionFive2 共用, 只差平台基地址。 */

#include <kernel/types.h>
#include <kernel/arch.h>
#include <kernel/print.h>
#include <platform.h>
#include <asm/csr.h>

/* ---- PLIC 寄存器地址计算 ---- */

/* 中断源优先级 (每个中断源 4 字节) */
#define PLIC_PRIORITY(irq)   (PLAT_PLIC_BASE + (irq) * 4)

/* S-mode 使能位图, 每个 hart 占 0x100 字节 */
#define PLIC_SENABLE(hart)   (PLAT_PLIC_BASE + 0x2080 + (hart) * 0x100)

/* S-mode 优先级阈值 */
#define PLIC_SPRIORITY(hart) (PLAT_PLIC_BASE + 0x201000 + (hart) * 0x2000)

/* S-mode claim/complete */
#define PLIC_SCLAIM(hart)    (PLAT_PLIC_BASE + 0x201004 + (hart) * 0x2000)

#define REG32(addr) (*(volatile uint32 *)(addr))

/* 由平台驱动列表决定该平台上有哪些中断源需要使能。
 * 这些宏由各平台的 platform.h 提供, 若平台没有该设备则定义为 0 个源。 */
#ifdef PLAT_UART0_IRQ
#define HAS_UART_IRQ 1
#else
#define HAS_UART_IRQ 0
#endif

#ifdef PLAT_VIRTIO0_IRQ
#define HAS_VIRTIO_IRQ 1
#else
#define HAS_VIRTIO_IRQ 0
#endif
static void plic_enable(int hart, int irq)
{
	REG32(PLIC_SENABLE(hart) + (irq / 32) * 4) |= (1u << (irq % 32));
}

/* --------------------------------------------------------------------------
 * 全局初始化: 设置各中断源的优先级
 * 只需在一个 hart 上执行一次。
 * -------------------------------------------------------------------------- */
void plic_init(void)
{
}

/* --------------------------------------------------------------------------
 * 每个 hart 的初始化: 使能中断源 + 设置优先级阈值
 * -------------------------------------------------------------------------- */
void plic_init_hart(void)
{
	int hartid = (int)arch_read_tp();

	/* 使能本 hart 关心的中断源。
	 * 注意: 这里用的是 S-mode 使能寄存器, 因为内核运行在 S-mode。 */
#if HAS_UART_IRQ
	plic_enable(hartid, PLAT_UART0_IRQ);
#endif
#if HAS_VIRTIO_IRQ
	plic_enable(hartid, PLAT_VIRTIO0_IRQ);
#endif

	/* 阈值设为 0, 表示接受所有优先级 > 0 的中断。
	 * 如果设成 1, 那么优先级为 1 的中断会被屏蔽掉 (要求严格大于阈值)。 */
	REG32(PLIC_SPRIORITY(hartid)) = 0;
}

/* --------------------------------------------------------------------------
 * 认领一个待处理的中断, 返回中断号 (0 表示没有中断)
 * -------------------------------------------------------------------------- */
int plic_claim(void)
{
}
void plic_complete(int irq)
{
}
