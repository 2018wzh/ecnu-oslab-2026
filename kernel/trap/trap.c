#include <kernel/trap.h>
#include <kernel/console.h>
#include <kernel/arch.h>
#include <kernel/print.h>
#include <drivers/irqchip/plic.h>
#include <drivers/serial/uart16550.h>
#include <platform.h>
#include <asm/csr.h>
/* 初始化 trap 中各个核心共享的东西；教师提供初始化框架。 */
void trap_init(void)
{
    plic_init(PLIC_BASE, UART_IRQ);
    timer_create();
    uart_enable_rx(UART_BASE, UART_SHIFT);
}
/* 初始化 trap 中各个核心独有的东西；所有依赖就绪后才打开中断。 */
void trap_inithart(void)
{
    extern void kernel_vector(void);
    csr_write(stvec, kernel_vector);
    plic_enable(PLIC_BASE, PLIC_CONTEXT(arch_hart_id()), UART_IRQ);
    timer_init();
    csr_write(sie, csr_read(sie) | (1UL << 5) | (1UL << 9));
    arch_irq_enable();
}
/* 在 kernel_vector 中调用：内核态 trap 处理的核心逻辑。
 * scause 是原因，frame->epc 是被打断的 PC，stval 是随原因变化的附加信息。
 * 先确认来源和中断状态，再区分中断、异常并分派；不要截断完整原因号。
 */
// TODO(lab-6): 时钟处理完成后，存在 Running 当前进程时 yield；恢复后保全原 PC/status。
void kernel_trap(trapframe_t *frame)
{
    uint64 sepc = frame->epc, sstatus = frame->status;
    uint64 scause = csr_read(scause), stval = csr_read(stval);
    assert(sstatus & (1UL << 8), "kernel_trap: not from s-mode");
    assert(!(csr_read(sstatus) & SSTATUS_SIE), "kernel_trap: interrupt enabled");
    /* 只去掉中断标志，保留其余所有原因位。 */
    uint64 trap_id = scause & ~(1UL << 63);
    if (scause & (1UL << 63)) {
        switch (trap_id) {
        // TODO(lab-3): 补充时钟和外部中断分支，处理后返回。
        default:
            printf("unexpected interrupt: cause=%p sepc=%p stval=%p\n",
                   (void *)(uintptr_t)trap_id, (void *)(uintptr_t)sepc, (void *)(uintptr_t)stval);
            panic("kernel_trap");
        }
    } else {
        switch (trap_id) {
        // TODO(lab-3): 分析异常原因；不能处理的异常保留下面的诊断。
        default:
            printf("unexpected exception: cause=%p sepc=%p stval=%p\n",
                   (void *)(uintptr_t)trap_id, (void *)(uintptr_t)sepc, (void *)(uintptr_t)stval);
            panic("kernel_trap");
        }
    }
}
// TODO(lab-3): claim、分派设备，再 complete；未知非零来源也要结束响应。
void external_interrupt(void) { panic("TODO(lab-3): external_interrupt"); }
/* 教师读取循环：键盘输入 -> 屏幕输出。
 * 学生在 external_interrupt 中识别 UART 来源并调用这里。
 */
void uart_interrupt(void)
{
    int c;
    // TODO(lab-3): 在教师读取循环中补充换行和 Backspace 的回显处理。
    while ((c = uart_getc(UART_BASE, UART_SHIFT)) != -1) console_putc((char)c);
}
