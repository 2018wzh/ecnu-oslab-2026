// 内核上下文的架构侧操作。
// 保存/恢复在 switch.S (必须用汇编), 这里放用 C 就能表达的架构相关操作。
#include <kernel/types.h>
#include <kernel/arch_context.h>
#include <kernel/string.h>
#include <kernel/print.h>

// 给从未运行过的新进程准备一个可信的入口。
void arch_context_init_for_new(context_t *ctx, void (*entry)(void),
                               uint64 kstack_top)
{
	// 清零: 新进程的 callee-saved 寄存器不应带有别人的值。
	memset(ctx, 0, sizeof(*ctx));

	// 把 sp 放到内核栈顶之下 (通常是 trapframe 起始地址)。
	// entry 从 sp 往下建栈帧, 不会覆盖栈顶的 trapframe。
	ctx->ra = (uint64)entry;
	ctx->sp = kstack_top;
}

// 上下文切换未实现时的落点。本分支的 `switch.S` 里 `arch_context_switch`
// 只留一条 `call` 到这里, 于是"还没做这一步"会表现为一句明确的信息。
void arch_context_switch_unimplemented(void)
{
	panic("上下文切换还没实现 (见本分支 README 的\"需要你完成的部分\")");
}
