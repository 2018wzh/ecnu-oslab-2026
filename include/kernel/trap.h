#ifndef OSLAB_TRAP_H
#define OSLAB_TRAP_H
#include <kernel/types.h>
/* 与 trap_entry.S 共用 272 字节布局；入口栈保持 16 字节对齐。
 * 内核与用户陷阱共用整数寄存器布局，不保存浮点/向量状态。
 */
typedef struct {
    uint64 x[32]; /* x[n] 位于 n*8；x[0]=0，x[2] 记录陷入前的 sp。 */
    uint64 epc;   /* 偏移 256：sepc，返回位置由事件语义决定。 */
    uint64 status;/* 偏移 264：陷入后的 sstatus，含 SPP/SPIE。 */
} trapframe_t;
_Static_assert(sizeof(trapframe_t) == 272, "trap assembly layout");
/* 启动核一次：共享 PLIC 优先级、ticks 和 UART 接收。 */
void trap_init(void);
/* 每核：向量、context、截止时间、源使能，最后开全局中断。 */
void trap_inithart(void);
/* frame 借用当前入口栈，返回前有效；不可保存到后续异步任务。 */
void kernel_trap(trapframe_t *frame);
void external_interrupt(void);
void uart_interrupt(void);
/* 每核设置绝对截止时间；SBI 失败则 panic。 */
void timer_init(void);
void timer_create(void);
void timer_update(void);
void timer_tick(void);
/* 同步读取系统滴答，不是直接读取硬件 time。 */
uint64 timer_ticks(void);
#endif
