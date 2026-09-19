// 加载磁盘上的 ELF 可执行文件 (generic kernel)。
// ELF 装载与"进程"是两件事: 前者解释文件格式, 后者把段映射进地址空间。
// ELF 头里 phoff/offset/filesz 全是不可信文件数据, 每处都要校验落在 elf_len
// 内, 否则损坏的 ELF 能让内核越界读。
#include <kernel/types.h>
#include <kernel/proc.h>
#include <kernel/mm.h>
#include <kernel/print.h>
#include <kernel/string.h>
#include <kernel/elf.h>
#include <kernel/arch.h>
#include <kernel/arch_mm.h>
#include <kernel/arch_trap.h>

/* 用户栈的大小与栈顶 (与 exec.c 的 exec_load_flat 保持一致)。
 * 栈顶来自 arch 层 —— 它由虚拟地址空间布局决定, 与具体开发板无关。 */
#define USER_STACK_SIZE  ARCH_USER_STACK_SIZE   /* 见 asm/pgtable.h */
#define USER_STACK_TOP   ARCH_USER_STACK_TOP

/* --------------------------------------------------------------------------
 * 校验 ELF 头部
 * -------------------------------------------------------------------------- */
static int elf_check(const elf_header_t *eh)
{
	if (*(const uint32 *)eh->ident != ELF_MAGIC) {
		printf("[exec] 不是 ELF 文件 (魔数错误)\n");
		return -1;
	}
	if (eh->type != 2) {
		printf("[exec] 不是可执行文件 (type=%d)\n", eh->type);
		return -1;
	}
	if (eh->machine != ARCH_ELF_MACHINE) {
		printf("[exec] 架构不匹配 (machine=%d, 期望 %d)\n",
		       eh->machine, ARCH_ELF_MACHINE);
		return -1;
	}
	if (eh->phnum == 0 || eh->phoff == 0) {
		printf("[exec] 没有程序头 (无法加载)\n");
		return -1;
	}
	return 0;
}
int exec_load(proc_t *p, const uint8 *elf, uint64 elf_len)
{
}
