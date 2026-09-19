#ifndef OSLAB_ARCH_H
#define OSLAB_ARCH_H
#include <kernel/types.h>
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
#endif
