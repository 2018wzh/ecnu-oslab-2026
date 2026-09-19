// QEMU virt 板级实现。
// 本文件是"这台机器上有哪些设备"(VirtIO/UART 地址与中断号) 这类知识的唯一存放
// 处; kernel/mm/vm.c 与 kernel/trap/trap.c 只调 platform_map_devices()/
// platform_dispatch_irq()。换板子改本文件而不是改 kernel/ 任何文件 (可移植性核心)。
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
	/* 启动核有两个数字值得对照: "平台配置"是 platform.h 里的预期值,
	 * "实际"是固件真正交给内核的那个 hart (由启动汇编抽签记录)。
	 * 正常情况两者相同; 不同就说明固件没按平台约定启动 —— 见 arch.h
	 * 里 arch_cpu_is_boot_hart 的说明。 */
	printf("[platform] cpus  : %d (启动核 = %lu, 平台配置 = %d)  uart  : 0x%lx (irq %d)\n",
	       PLAT_NCPU, (uint64)arch_cpu_cold_boot_hart(), PLAT_BOOT_HART,
	       (uint64)PLAT_UART0_BASE, PLAT_UART0_IRQ);
}

extern void _entry(void);
uint64 platform_secondary_entry(void)
{
	return (uint64)_entry;
}

/* --------------------------------------------------------------------------
 * 映射本平台的设备 MMIO 区域
 *
 * QEMU virt 上需要映射两组:
 *   1. PLIC 与 UART  (从 PLAT_PLIC_BASE 到 PLAT_UART0_BASE + 一页)
 *   2. VirtIO MMIO   (8 个槽位, 每个一页)
 * -------------------------------------------------------------------------- */
int platform_map_devices(uint64 pgtbl)
{
	/* PLIC + UART 区域 */
	uint64 a_begin = ALIGN_DOWN(PLAT_PLIC_BASE, PGSIZE);
	uint64 a_end   = ALIGN_UP(PLAT_UART0_BASE + PGSIZE, PGSIZE);
	if (kvm_map(pgtbl, a_begin, a_begin, a_end - a_begin, PERM_R | PERM_W) < 0)
		return -1;

	/* VirtIO MMIO 区域。PLAT_VIRTIO_COUNT 由本平台的头文件给出,
	 * 所以这里不需要任何条件编译 —— 需要映射多少槽位是"本机器"的事实。 */
	uint64 v_begin = ALIGN_DOWN(PLAT_VIRTIO0_BASE, PGSIZE);
	uint64 v_end   = ALIGN_UP(PLAT_VIRTIO0_BASE + PLAT_VIRTIO_COUNT * PGSIZE, PGSIZE);
	if (kvm_map(pgtbl, v_begin, v_begin, v_end - v_begin, PERM_R | PERM_W) < 0)
		return -1;

	printf("[platform] 已映射设备区 [0x%lx, 0x%lx) 与 VirtIO [0x%lx, 0x%lx)\n",
	       a_begin, a_end, v_begin, v_end);
	return 0;
}

/* --------------------------------------------------------------------------
 * 把外部中断号分发给对应设备
 * -------------------------------------------------------------------------- */
void uart_intr(void);
void virtio_disk_intr(void);

int platform_dispatch_irq(int irq)
{
	if (irq == PLAT_UART0_IRQ) {
		uart_intr();
		return 1;
	}
	if (irq == PLAT_VIRTIO0_IRQ) {
		virtio_disk_intr();
		return 1;
	}
	return 0;
}
