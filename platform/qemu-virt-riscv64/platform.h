// QEMU `virt` 机器的板级资源描述。
// 下面这些地址 (0x10000000, 0x0c000000...) 是"QEMU virt 这台机器"的属性, 不是
// RISC-V ISA 属性 (真机 JH7110 把 PLIC/UART 放在完全不同的地址), 所以属 platform
// 层。规则: kernel/ 不允许写死 MMIO 地址或 include 具体平台目录, 只能经
// <platform.h> 拿资源; 构建系统用 -I$(PLATFORM_DIR) 决定它解析到哪台机器。
#ifndef __PLATFORM_H__
#define __PLATFORM_H__

/* --------------------------------------------------------------------------
 * 1. 物理内存布局
 * -------------------------------------------------------------------------- */

/* DRAM 起始地址。QEMU virt 的物理内存从 0x80000000 开始。 */
#define PLAT_DRAM_BASE      0x80000000UL

/* DRAM 容量 (必须与 QEMU 的 -m 参数一致, 见 configs/ 下的 .mk 的 QEMU_MEM)。
 * 内核只会管理 [PLAT_DRAM_BASE, PLAT_DRAM_BASE + PLAT_DRAM_SIZE) 这段内存。 */
#define PLAT_DRAM_SIZE      (128UL * 1024 * 1024)

/* 内核被加载到的物理地址。
 * OpenSBI 占据 [0x80000000, 0x80200000), 运行完跳转到 0x80200000。
 * 这个值与链接脚本 arch/riscv64/linker/kernel.ld 以及 configs 中的
 * KERNEL_LOAD_ADDR 必须完全一致, 三者对不上内核会直接跑飞。 */
#define PLAT_KERNEL_BASE    0x80200000UL

/* 保留区: 内核镜像 + OpenSBI 固件占用的区间不参与物理页分配。
 * 注意 PLAT_FIRMWARE_BASE 是 OpenSBI 所在位置。 */
#define PLAT_FIRMWARE_BASE  0x80000000UL

/* --------------------------------------------------------------------------
 * 2. CPU 拓扑
 * -------------------------------------------------------------------------- */

/* 该平台上参与运行内核的 hart 数量。
 * 必须与 QEMU 的 -smp 一致, 否则内核会尝试启动不存在的 hart。 */
#define PLAT_NCPU           2

/* 第一个用来运行内核的 hart id。
 * QEMU 从 hart 0 启动, 所有 hart 都可用, 所以就是 0。
 * 对比 VisionFive2: 那里 hart 0 是监控核, 内核只能用 1..4。 */
#define PLAT_BOOT_HART      0

/* 内核可用的 hart id 区间 (闭区间)。
 * 启动代码用它来校验固件传进来的 hartid 是否合法 ——
 * 必须用区间而不是 "hartid < PLAT_NCPU", 因为两个平台的核心
 * 编号起点不同 (QEMU 从 0 开始, VF2 从 1 开始)。
 *
 * 这两个宏与 PLAT_NCPU 必须满足: PLAT_HART_MAX - PLAT_HART_MIN + 1 == PLAT_NCPU
 * 关系写错了症状是"少起来一个核"或者内核踩到别的核的栈。 */
#define PLAT_HART_MIN       0
#define PLAT_HART_MAX       1

/* --------------------------------------------------------------------------
 * 3. 中断控制器
 * -------------------------------------------------------------------------- */

/* PLIC (Platform-Level Interrupt Controller), 负责外部设备中断。
 * 这个控制器是 SiFive 的设计, 很多 RISC-V SoC 都用它,
 * 所以 drivers/irqchip/plic.c 的代码可以跨平台复用, 只有基地址不同。 */
#define PLAT_PLIC_BASE      0x0c000000UL
#define PLAT_PLIC_SIZE      0x4000000UL
#define PLAT_TIMER_INTERVAL 1000000UL

/* --------------------------------------------------------------------------
 * 5. 串口 (console)
 * -------------------------------------------------------------------------- */

/* QEMU virt 的 16550A UART。
 * 16550 是业界非常通用的串口控制器, 因此 uart16550.c 可以直接复用到
 * VisionFive2 —— 那边也是 16550 兼容的 UART, 只是地址不同。 */
#define PLAT_UART0_BASE     0x10000000UL
#define PLAT_UART0_IRQ      10
#define PLAT_UART0_CLOCK    3686400UL   /* 输入时钟, 用于计算分频 */

/* --------------------------------------------------------------------------
 * 6. 块设备 (磁盘)
 * -------------------------------------------------------------------------- */

/* QEMU virt 用 VirtIO-MMIO 提供磁盘。
 * 对比 VisionFive2: 那里是真实的 SD 卡, 走 DW MSHC (SDHCI) 控制器。
 * 两者的上层接口相同 (都实现 block_device 的 read/write),
 * 所以文件系统完全不需要知道底下是 VirtIO 还是 SD 卡。 */
#define PLAT_VIRTIO0_BASE   0x10001000UL
#define PLAT_VIRTIO0_IRQ    1
#define PLAT_VIRTIO_COUNT   8           /* virt 机器上的 virtio-mmio 槽位数 */

/* --------------------------------------------------------------------------
 * 7. 平台标识 (仅供调试打印)
 * -------------------------------------------------------------------------- */
#define PLAT_NAME           "qemu-virt-riscv64"

#endif /* __PLATFORM_H__ */
