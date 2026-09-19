// generic trap 处理逻辑。这里放与架构无关的决策: 时钟中断怎么做 (更新计数、
// 唤醒睡眠、触发调度)、外部中断怎么做 (claim->分发给设备->complete)、缺页怎么
// 处理。arch 层只负责"告诉我们是哪种 trap", 具体怎么做由这里决定。
#include <kernel/types.h>
#include <kernel/print.h>
#include <kernel/arch.h>
#include <kernel/arch_trap.h>

/* 各子系统的接口 */
#include <kernel/timer.h>
#include <kernel/platform.h>

/* 实验阶段开关 (由构建系统定义; 未定义时等于 9 = 完整实现) */

int      plic_claim(void);
void     plic_complete(int irq);
void     uart_intr(void);
void     virtio_disk_intr(void);
void     proc_yield(void);

/* --------------------------------------------------------------------------
 * 时钟中断
 * -------------------------------------------------------------------------- */
void trap_handle_timer(void)
{
}

/* --------------------------------------------------------------------------
 * 外部中断 (来自 PLIC 的设备中断)
 * -------------------------------------------------------------------------- */
void trap_handle_external(void)
{
}

/* --------------------------------------------------------------------------
 * 缺页异常
 * -------------------------------------------------------------------------- */
void trap_handle_page_fault(uint64 cause, uint64 stval, uint64 sepc)
{
	printf("\n[pagefault] 用户态缺页:\n");
	printf("  类型   : %s\n",
	       cause == 12 ? "取指缺页" : (cause == 13 ? "读缺页" : "写缺页"));
	printf("  地址   : 0x%lx\n", stval);
	printf("  指令 PC: 0x%lx\n", sepc);
	printf("  说明   : lab-4 之后, 合法的 mmap 区域会在这里按需分配物理页;\n");
	printf("           非法访问应当终止该进程而不是让内核崩溃。\n");
	panic("用户态缺页 (lab-4 实现按需分页后此路径应被正确处理)");
}
