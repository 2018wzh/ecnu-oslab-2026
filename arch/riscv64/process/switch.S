// 内核上下文切换。
// void arch_context_switch(struct arch_context *old, struct arch_context *new)
//   a0 = old   (保存当前上下文),   a1 = new  (从这里恢复)
// 该函数"返回两次": 第一次在调用者调用并保存 old 后, 第二次是将来别的进程
// 切回 old 时经由 old->ra 返回 —— 所以从调用者看它一次调用会被"暂停"。
#include <asm/context.h>

.section .text
.global arch_context_switch
.align 4
arch_context_switch:
        // ---- 1. 保存当前上下文到 old (a0) ----
        call    arch_context_switch_unimplemented
