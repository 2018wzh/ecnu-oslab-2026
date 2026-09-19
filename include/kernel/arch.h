/* 架构抽象接口 (arch-api): generic kernel 通过它访问 CPU, 不 include 具体架构路径。
 * CPU 架构在编译期唯一确定, 故用普通函数声明由链接期选定实现, 而非函数指针表。 */
#ifndef __KERNEL_ARCH_H__
#define __KERNEL_ARCH_H__

#include <kernel/types.h>

// CPU 编号
// 当前 hart 的 CPU 逻辑编号 (从 0 开始), 注意不是 hartid; 换算由 arch 层负责。
int arch_cpu_id(void);

// hartid <-> cpuid 换算 (平台差异由 PLAT_BOOT_HART 描述)。
int    arch_hart_to_cpuid(uint64 hartid);
uint64 arch_cpuid_to_hart(int cpuid);

// 该 hartid 是否由内核使用 (用于校验固件传来的参数)。
bool arch_hart_is_valid(uint64 hartid);

// 冷启动核的 hartid 与"我是不是它"。
// 固件不一定照办平台常量, 故启动汇编用原子交换抽签, 这里读抽签结果。
uint64 arch_cpu_cold_boot_hart(void);
bool   arch_cpu_is_boot_hart(void);

// 抽签是否真的发生过; 为 false 说明启动汇编那一步没走到, 内核处于未定义状态。
bool   arch_cpu_boot_claim_ok(void);

// 中断控制
// enable/disable 只影响当前 CPU, 不是全局开关; save/restore 必须配对使用,
// restore 恢复到 save 时状态, 而非无条件开中断。
void   arch_irq_enable(void);
void   arch_irq_disable(void);
uint64 arch_irq_save(void);
void   arch_irq_restore(uint64 flags);
int    arch_irq_is_enabled(void);

// 时间: 读取 CPU 时间计数器 (单位: tick, 频率由平台决定)。
uint64 arch_read_time(void);

// 早期控制台: 直接把字符串送到固件控制台 (RISC-V 上是 SBI 控制台)。
// 用于 UART 驱动初始化之前或分页刚开启之后, 不经过我们的页表; 仅启动早期使用。
void arch_early_puts(const char *s);

#endif /* __KERNEL_ARCH_H__ */
