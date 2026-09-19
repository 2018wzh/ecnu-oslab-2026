// VisionFive2 (JH7110) 板级实现。
// 与 platform/qemu-virt-riscv64/platform.c 结构完全一样, 只内容不同 (设备区域、
// 中断号)。kernel/ 下的代码看不到这些差异, 只调 platform_map_devices()/
// platform_dispatch_irq() —— 加第三块板子只需再写一个这样的文件。
#include <kernel/types.h>
#include <kernel/print.h>
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
