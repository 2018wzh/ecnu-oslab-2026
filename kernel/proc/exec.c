// 加载并执行用户程序 (generic kernel)。
// exec 做: 校验 ELF -> 建全新用户页表 -> 逐段映射并拷贝 -> 分配用户栈 ->
// 设置 trapframe, 使"从 trap 返回"时直接进用户态。
// 最后一步最巧妙: 把 sepc/sp 设为入口和用户栈顶后走 trap_return_to_user 的 sret,
// 于是"首次进用户态"和"系统调用返回"走的是同一段代码 —— 理解 trap 的关键。
#include <kernel/types.h>
#include <kernel/proc.h>
#include <kernel/mm.h>
#include <kernel/print.h>
#include <kernel/string.h>
#include <kernel/arch.h>
#include <kernel/arch_trap.h>
#include <kernel/arch_mm.h>
#include <uapi/syscall.h>

#define USER_STACK_SIZE  ARCH_USER_STACK_SIZE   /* 见 asm/pgtable.h */
#define USER_STACK_TOP   ARCH_USER_STACK_TOP

void trap_return_to_user(trapframe_t *tf);
void user_enter(proc_t *p)
{
	trapframe_t *tf = p->tf;
	proc_set_current(p);
	p->state = PROC_RUNNING;
	p->cpuid = arch_cpu_id();
	if (!p->ofile[STDOUT_FILENO]) {
		if (fd_setup_stdio(p) < 0)
			panic("user_enter: 无法为用户进程建立标准输入输出");
	}

	/* 切换到该进程的页表 */
	arch_mmu_activate(p->pgtbl);
	uint64 kstack_top = p->kstack + KSTACK_SIZE;
	arch_set_kernel_stack(kstack_top);

	trap_return_to_user(tf);

	/* 不应该到这里 */
	for (;;)
		;
}
int exec_load_flat(proc_t *p, const uint8 *img, uint64 len, uint64 vaddr,
                   int perm, uint64 stack_top, uint64 entry)
{
	pgtbl_t newpgtbl = kvm_copy_kernel(kvm_global());
	if (!newpgtbl) {
		printf("[exec] 无法创建页表\n");
		return -1;
	}

	/* 把整段镜像按页映射到固定虚拟地址。
	 * 大小向上取整到页 —— 因为映射的最小单位是页。 */
	uint64 mapsize = ALIGN_UP(len, PGSIZE);
	if (uvm_alloc(newpgtbl, vaddr, mapsize, perm) == 0) {
		printf("[exec] 无法映射代码段\n");
		return -1;
	}
	if (uvm_copyout(newpgtbl, vaddr, (uint64)img, len) < 0) {
		printf("[exec] 无法拷贝代码段\n");
		return -1;
	}

	/* 用户栈 */
	uint64 stack_bottom = stack_top - USER_STACK_SIZE;
	if (uvm_alloc(newpgtbl, stack_bottom, USER_STACK_SIZE,
	              PERM_R | PERM_W | PERM_U) == 0) {
		printf("[exec] 用户栈分配失败\n");
		return -1;
	}

	p->pgtbl = newpgtbl;
	p->sz = vaddr + mapsize;

	trapframe_t *tf = p->tf;
	/* 先清零: 新程序不应该读到上一个程序留在寄存器里的任何东西,
	 * 而 memset + 逐项清零的写法里, "哪些字段要清" 取决于架构的
	 * 寄存器个数 —— 那是 arch 层的事, 所以用 TF_ZERO。 */
	TF_ZERO(tf);
	TF_SET_PC(tf, entry);
	TF_SET_USER_SP(tf, stack_top);

	printf("[exec] 已加载扁平镜像: %lu 字节 @ 0x%lx, 入口 0x%lx\n",
	       len, vaddr, entry);
	return 0;
}
