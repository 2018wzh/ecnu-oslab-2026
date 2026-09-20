#ifndef OSLAB_ARCH_H
#define OSLAB_ARCH_H
#include <kernel/types.h>
/* CPU 身份、S-mode 状态与嵌套中断操作；平台参数决定 hart 到 cpuid 的映射。 */
uint64 arch_hart_id(void);
uint64 arch_cpu_id(void);
bool arch_is_boot_cpu(void);
int arch_start_cpu(uint64 cpu);
bool arch_irq_enabled(void);
void arch_irq_disable(void);
void arch_irq_enable(void);
void arch_early_init(void);
void arch_park(void) __attribute__((noreturn));
void push_off(void);
void pop_off(void);
uint64 arch_kernel_satp(void);
void arch_user_return(uint64 user_satp, uint64 pc) __attribute__((noreturn));
#endif
