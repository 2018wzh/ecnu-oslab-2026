/* 内核上下文 (context) 的通用视图: 保存 callee-saved 寄存器,
 * 定义在 arch 层, generic kernel 只用这个类型名与上下文切换接口。 */
#ifndef __KERNEL_ARCH_CONTEXT_H__
#define __KERNEL_ARCH_CONTEXT_H__

#include <asm/context.h>

// 内核上下文, generic kernel 只声明变量、不读写字段。
typedef struct arch_context context_t;

// 让从未运行过的新进程首次被调度时从 entry 开始, 并保证 entry 压栈不覆盖栈顶数据。
void arch_context_init_for_new(context_t *ctx, void (*entry)(void),
                               uint64 kstack_top);

#endif /* __KERNEL_ARCH_CONTEXT_H__ */
