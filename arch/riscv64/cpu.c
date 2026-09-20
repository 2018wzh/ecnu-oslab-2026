/* 每核身份和带嵌套计数的中断开关。 */
#include <kernel/arch.h>
#include <kernel/print.h>
#include <asm/csr.h>
#include <platform.h>
uint64 boot_hart;
/* 每核只访问自己的状态：depth 是关闭深度，enabled 保存首次关闭前状态。 */
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
/* 开关中断的基本逻辑：
 * 1. 多个地方可能开关中断，因此它不是“开/关”的二元状态，
 *    而是“关 关 关 开 开 开”的 stack。
 * 2. 在第一次执行关中断时，记录中断的初始状态为 X。
 * 3. 每次关中断，stack 中的元素加 1。
 * 4. 每次开中断，stack 中的元素减 1。
 * 5. 如果 stack 中元素清空，将中断状态设为初始的 X。
 */
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

/* 教师切换外围：只在关闭中断且唯一持锁时访问本核恢复策略。 */
unsigned arch_interrupt_depth(void)
{
    assert(!arch_irq_enabled(), "interrupt depth: interrupts enabled");
    return irq_state[arch_cpu_id()].depth;
}
bool arch_resume_interrupts(void)
{
    assert(arch_interrupt_depth() == 1, "resume interrupts: nesting");
    return irq_state[arch_cpu_id()].enabled;
}
void arch_set_resume_interrupts(bool enabled)
{
    assert(arch_interrupt_depth() == 1, "resume interrupts: nesting");
    irq_state[arch_cpu_id()].enabled = enabled;
}
