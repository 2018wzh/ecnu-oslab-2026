// 内核上下文 (架构相关)。
// 上下文切换发生在内核态 (内核线程让出 CPU 另一接着跑)。它是同步函数调用
// (arch_context_switch) 触发的, 调用者已按 ABI 把 caller-saved 保存进自己栈帧,
// 所以只需保存 callee-saved: ra (切换回来继续执行)、sp (每进程有自己的内核栈)、
// s0-s11。这就是 context 结构体这么小的原因。
// 对比 trapframe: trap 是硬件异步触发, 无"调用者已保存"前提, 要存全部寄存器;
// context 只存 callee-saved。理解这个区别是理解上下文切换的关键。
#ifndef __ASM_CONTEXT_H__
#define __ASM_CONTEXT_H__

#ifndef __ASSEMBLER__
#include <kernel/types.h>

struct arch_context {
	uint64 ra;        /* 返回地址 */
	uint64 sp;        /* 栈指针 */
	uint64 s0;        /* 以下为 callee-saved 寄存器 */
	uint64 s1;
	uint64 s2;
	uint64 s3;
	uint64 s4;
	uint64 s5;
	uint64 s6;
	uint64 s7;
	uint64 s8;
	uint64 s9;
	uint64 s10;
	uint64 s11;
};

STATIC_ASSERT(sizeof(struct arch_context) == 14 * 8,
              "arch_context 大小与 CTX_* 偏移宏不一致");
#endif /* __ASSEMBLER__ */
#define CTX_RA   (0 * 8)
#define CTX_SP   (1 * 8)
#define CTX_S0   (2 * 8)
#define CTX_S1   (3 * 8)
#define CTX_S2   (4 * 8)
#define CTX_S3   (5 * 8)
#define CTX_S4   (6 * 8)
#define CTX_S5   (7 * 8)
#define CTX_S6   (8 * 8)
#define CTX_S7   (9 * 8)
#define CTX_S8   (10 * 8)
#define CTX_S9   (11 * 8)
#define CTX_S10  (12 * 8)
#define CTX_S11  (13 * 8)

#endif /* __ASM_CONTEXT_H__ */
