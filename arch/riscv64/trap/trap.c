// 架构相关 trap 分发 (arch 层)。
// 本文件只做"翻译": 读 scause 判断中断还是异常, 再交给 generic 层的处理函数;
// 具体"时钟中断要做什么"由 generic 决定, 换架构只改 scause 编码, generic 不动。
#include <kernel/types.h>
#include <kernel/print.h>
#include <kernel/arch.h>
#include <asm/csr.h>
#include <asm/trapframe.h>

/* generic 层的处理函数 */
void trap_handle_timer(void);
void trap_handle_external(void);
uint64 trap_handle_syscall(struct trapframe *tf);
void trap_handle_page_fault(uint64 cause, uint64 stval, uint64 sepc);

/* --------------------------------------------------------------------------
 * 安装 trap 向量
 * -------------------------------------------------------------------------- */
void trap_arch_init(void)
{
	extern void trap_entry(void);
	/* stvec 的低 2 位是模式选择:
	 *   0 = Direct   : 所有 trap 都跳到同一个地址
	 *   1 = Vectored : 中断跳到 base + 4*cause
	 * 这里用 Direct, 由 C 代码自己分发, 更灵活也更容易调试。 */
	arch_write_stvec((uint64)trap_entry & ~0x3UL);
}
void arch_set_kernel_stack(uint64 kstack_top)
{
	asm volatile("csrw sscratch, %0" : : "r"(kstack_top) : "memory");
}

/* 取出 trapframe 中某个寄存器的值 (供 generic 层读取系统调用参数) */
uint64 trapframe_get_reg(struct trapframe *tf, int idx)
{
	if (idx < 0 || idx >= 31)
		return 0;
	return tf->regs[idx];
}

void trapframe_set_reg(struct trapframe *tf, int idx, uint64 v)
{
	if (idx >= 0 && idx < 31)
		tf->regs[idx] = v;
}

uint64 trapframe_get_sepc(struct trapframe *tf) { return tf->sepc; }
void   trapframe_set_sepc(struct trapframe *tf, uint64 v) { tf->sepc = v; }
void arch_trap_unimplemented(void)
{
	panic("用户态 trap 的入口/返回还没实现 (见本分支 README 的\"需要你完成的部分\")");
}

void trap_user_handler(struct trapframe *tf)
{
}

/* --------------------------------------------------------------------------
 * 内核态 trap 处理
 * -------------------------------------------------------------------------- */
void trap_kernel_handler(struct trapframe *tf)
{
	uint64 scause = arch_read_scause();
	uint64 stval  = arch_read_stval();

	if (scause & SCAUSE_INTERRUPT) {
		switch (scause & SCAUSE_CODE_MASK) {
		case SCAUSE_S_TIMER:
			trap_handle_timer();
			break;
		case SCAUSE_S_EXTERNAL:
			trap_handle_external();
			break;
		default:
			break;
		}
	} else {
		/* 内核态异常基本都是 bug, 直接崩溃并打印足够的信息 */
		printf("\n[trap] 内核态异常:\n");
		printf("  scause = 0x%lx (code %lu)\n", scause,
		       scause & SCAUSE_CODE_MASK);
		printf("  stval  = 0x%lx\n", stval);
		printf("  sepc   = 0x%lx\n", tf->sepc);
		printf("  提示: 内核态缺页通常是页表映射不全, 请检查 kvm_init()\n");
		panic("未处理的内核态异常");
	}
}
