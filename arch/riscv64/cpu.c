/* 每核身份和带嵌套计数的中断开关。 */
#include <kernel/arch.h>
#include <kernel/print.h>
#include <asm/csr.h>
#include <platform.h>
uint64 boot_hart;
static struct { unsigned depth; bool enabled; } irq_state[NCPU];
uint64 arch_hart_id(void) { uint64 x; __asm__ volatile("mv %0, tp" : "=r"(x)); return x; }
uint64 arch_cpu_id(void) { return arch_hart_id() - HART_FIRST; }
bool arch_is_boot_cpu(void) { return arch_hart_id() == boot_hart; }
bool arch_irq_enabled(void) { return (csr_read(sstatus) & SSTATUS_SIE) != 0; }
void arch_irq_disable(void) { __asm__ volatile("csrci sstatus, 2" ::: "memory"); }
void arch_irq_enable(void) { __asm__ volatile("csrsi sstatus, 2" ::: "memory"); }
void arch_early_init(void)
{
    extern void early_trap(void);
    arch_irq_disable();
    csr_write(sie, 0);
    csr_write(satp, 0);
    csr_write(stvec, early_trap);
}
void arch_park(void) { for (;;) __asm__ volatile("wfi"); }
void push_off(void)
{
    bool old = arch_irq_enabled();
    arch_irq_disable();
    uint64 cpu = arch_cpu_id();
    if (irq_state[cpu].depth == 0) irq_state[cpu].enabled = old;
    irq_state[cpu].depth++;
}
void pop_off(void)
{
    uint64 cpu = arch_cpu_id();
    if (arch_irq_enabled() || irq_state[cpu].depth == 0) panic("unbalanced pop_off");
    if (--irq_state[cpu].depth == 0 && irq_state[cpu].enabled) arch_irq_enable();
}
