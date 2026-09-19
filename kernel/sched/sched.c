// 调度器 (generic kernel)。
// 机制与策略分离: 策略 (本文件) 决定"下一个该运行谁" (轮转, 与架构无关),
// 机制 (arch_context_switch) 保存/恢复寄存器。换优先级调度只需改 pick_next()。
// 每个 CPU 一个 idle 进程: 无就绪进程时必须有人跑, 否则 sched_switch 无处可去。
// idle 只执行 wfi (低功耗 + 可被时钟中断唤醒)。多核并行, 不能用全局 idle。
#include <kernel/types.h>
#include <kernel/proc.h>
#include <kernel/print.h>
#include <kernel/arch.h>
#include <kernel/mm.h>
#include <kernel/string.h>

/* 由 proc.c 提供 */
void proc_set_current(proc_t *p);

/* --------------------------------------------------------------------------
 * 每个 CPU 的 idle 上下文
 *
 * idle 也需要一个 context, 因为它参与上下文切换:
 * 用户进程让出 -> 切到 idle -> 时钟中断 -> 切回用户进程
 * -------------------------------------------------------------------------- */
static context_t idle_ctx[N_CPU_MAX];


// 调度器全局锁: 保护"挑一个就绪进程"+"把状态改成 RUNNING"这两步原子。
// 用静态初始化而非 spinlock_init: 从核可能先于启动核走到调度路径, 静态初始化
// 没有"启动核清零锁"的时序窗口。
static proc_t idle_proc[N_CPU_MAX];

/* idle 进程的内核栈 (静态分配, 不能用 pmem_alloc 因为 idle 可能
 * 在内存分配器初始化之前就存在) */
static uint8 idle_stack[N_CPU_MAX][KSTACK_SIZE] __attribute__((aligned(16)));

/* --------------------------------------------------------------------------
 * idle 线程主体: 只是等待中断
 * -------------------------------------------------------------------------- */
static void idle_main(void)
{
	printf("[sched] cpu %d 已切换到 idle 进程 (上下文切换生效)\n", arch_cpu_id());

	for (;;) {
		// 开中断等 wfi, 醒后关调度; 状态读写要原子。
		arch_irq_enable();
		asm volatile("wfi");       // 等中断 (通常是时钟中断)
		arch_irq_disable();
		sched_switch();
	}
}

/* --------------------------------------------------------------------------
 * 初始化当前 CPU 的 idle 进程
 * -------------------------------------------------------------------------- */
void sched_init_hart(void)
{
	int id = arch_cpu_id();
	if (id < 0 || id >= N_CPU_MAX)
		panic("sched_init_hart: 非法 cpuid %d", id);

	proc_t *p = &idle_proc[id];
	memset(p, 0, sizeof(*p));
	p->pid = 0;                    /* pid 0 保留给 idle */
	p->state = PROC_RUNNABLE;
	p->cpuid = id;
	p->kstack = (uint64)idle_stack[id];
	strcpy(p->name, "idle");

	/* 构造 idle 的初始上下文。
	 * 我们要让 arch_context_switch 第一次切到 idle 时,
	 * 从 idle_main 开始执行。
	 *
	 * 做法: 手工"伪造"一个刚被切换出去的上下文:
	 *   ra    = idle_main      (切换后 ret 会跳到这里)
	 *   sp    = 内核栈顶        (注意是栈顶! 栈向下增长)
	 * 这就是"创建一个从未运行过的上下文"的标准技巧。
	 * 学生应当理解: 上下文切换不要求目标真的运行过, 只要
	 * 它的 ra/sp 被正确设置, 就能"返回"到一个新函数。 */
	idle_ctx[id].ra = (uint64)idle_main;
	idle_ctx[id].sp = (uint64)idle_stack[id] + KSTACK_SIZE;
	for (int i = 0; i < 14; i++)
		((uint64 *)&idle_ctx[id])[i] = 0;
	idle_ctx[id].ra = (uint64)idle_main;
	idle_ctx[id].sp = (uint64)idle_stack[id] + KSTACK_SIZE;

	p->ctx = idle_ctx[id];

	proc_set_current(p);
}
static proc_t *pick_next(void)
{
}
void sched_switch(void)
{
}
