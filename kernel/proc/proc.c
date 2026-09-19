// 进程管理 (generic kernel)。
// 核心数据结构: proc_table[NPROC] 存所有进程 (含未用槽位),
// cpu_proc[cpuid] 记录每个 CPU 正在运行的进程。
// "当前进程"必须按 CPU 分存: 多核真并行, 用全局 current 会让一个 CPU 看到
// 另一 CPU 的当前进程而踩坏对方现场。
#include <kernel/types.h>
#include <kernel/proc.h>
#include <kernel/print.h>
#include <kernel/string.h>
#include <kernel/mm.h>
#include <kernel/arch.h>
#include <kernel/fs.h>
#include <kernel/arch_mm.h>
#include <uapi/syscall.h>

static proc_t proc_table[NPROC];
static proc_t *cpu_proc[N_CPU_MAX];
static spinlock_t proc_lock;
/* wait 的全局锁, 理由见 include/kernel/proc.h 的 proc_wait_lock。 */
static spinlock_t wait_lock;
static int next_pid = 1;

/* 空闲的内核栈数组 (由 entry.S 和各进程分配) */

void proc_init(void)
{
	spinlock_init(&proc_lock, "proc");
	spinlock_init(&wait_lock, "wait");
	memset(proc_table, 0, sizeof(proc_table));
	memset(cpu_proc, 0, sizeof(cpu_proc));
	next_pid = 1;

	for (int i = 0; i < NPROC; i++) {
		proc_table[i].state = PROC_UNUSED;
		proc_table[i].pid = 0;
		proc_table[i].cpuid = -1;
		spinlock_init(&proc_table[i].lk, "proc");
	}

	printf("[proc] 进程表已初始化 (%d 个槽位)\n", NPROC);
}

/* --------------------------------------------------------------------------
 * 分配一个进程槽位
 * -------------------------------------------------------------------------- */
proc_t *proc_alloc(void)
{
	spinlock_acquire(&proc_lock);
	for (int i = 0; i < NPROC; i++) {
		if (proc_table[i].state == PROC_UNUSED) {
			proc_t *p = &proc_table[i];
			/* 清零, 但保留自旋锁 (它已经被初始化过, 且可能正被使用) */
			spinlock_t saved = p->lk;
			memset(p, 0, sizeof(*p));
			p->lk = saved;
			p->pid = next_pid++;
			p->state = PROC_UNUSED;   /* 调用者负责设置为 RUNNABLE */
			p->cpuid = -1;
			spinlock_release(&proc_lock);
			return p;
		}
	}
	spinlock_release(&proc_lock);
	return NULL;
}

/* --------------------------------------------------------------------------
 * 当前进程
 * -------------------------------------------------------------------------- */
proc_t *myproc(void)
{
	int id = arch_cpu_id();
	if (id < 0 || id >= N_CPU_MAX)
		return NULL;
	/* 关中断读取: 保证读到的 cpu_proc 在返回后仍然属于本 CPU
	 * (否则可能在读完之后立刻被调度走, 返回一个错误的进程) */
	uint64 flags = arch_irq_save();
	proc_t *p = cpu_proc[id];
	arch_irq_restore(flags);
	return p;
}

proc_t *proc_of_cpu(int cpuid)
{
	if (cpuid < 0 || cpuid >= N_CPU_MAX)
		return NULL;
	return cpu_proc[cpuid];
}

void proc_set_current(proc_t *p)
{
	cpu_proc[arch_cpu_id()] = p;
}
void sched_switch(void);

void proc_sleep(void *chan, spinlock_t *lk)
{
}

void proc_wakeup(void *chan)
{
}

spinlock_t *proc_wait_lock(void)
{
	return &wait_lock;
}

/* --------------------------------------------------------------------------
 * 让出 CPU
 * -------------------------------------------------------------------------- */
void proc_yield(void)
{
	sched_switch();
}

/* --------------------------------------------------------------------------
 * 进程退出
 * -------------------------------------------------------------------------- */
void proc_exit(int status)
{
}

/* --------------------------------------------------------------------------
 * 回收进程资源
 * -------------------------------------------------------------------------- */
void proc_free(proc_t *p)
{
	if (!p)
		return;

	spinlock_acquire(&proc_lock);
	p->state = PROC_UNUSED;
	p->pid = 0;
	p->parent = NULL;
	p->pgtbl = 0;
	p->sz = 0;
	spinlock_release(&proc_lock);

	/* 注意: 内核栈与页表的释放需要 uvm_free (lab-4 实现)。
	 * 这里把结构体标为空闲即可, 因为 proc_alloc 会重新清零。 */
}
proc_t *proc_table_base(void)
{
	return proc_table;
}

void proc_dump(void)
{
	static const char *state_name[] = {
		"UNUSED", "RUNNABLE", "RUNNING", "SLEEPING", "ZOMBIE"
	};

	printf("pid  cpu  state     name\n");
	for (int i = 0; i < NPROC; i++) {
		proc_t *p = &proc_table[i];
		if (p->state == PROC_UNUSED)
			continue;
		printf("%3d  %3d  %-9s %s\n", p->pid, p->cpuid,
		       state_name[p->state], p->name);
	}
}
static int proc_copy_user_range(proc_t *np, proc_t *p,
                                uint64 va_start, uint64 va_end)
{
	for (uint64 va = va_start; va < va_end; va += PGSIZE) {
		uint64 old_pa = kvm_translate(p->pgtbl, va);
		if (old_pa == 0)
			continue;   /* 父进程这里没映射, 子进程也不需要 */

		uint64 new_pa = pmem_alloc_user(true);
		if (!new_pa) {
			printf("[fork] 内存不足, 无法复制地址空间\n");
			return -1;
		}

		memmove((void *)new_pa, (void *)old_pa, PGSIZE);

		/* 权限保持"用户可访问 + 可读写执行": 父进程的原始权限没有
		 * 保存在页表项之外。学生可以作为改进点: 遍历时解析 PTE 的
		 * 权限位并原样复制。 */
		if (kvm_map(np->pgtbl, va, new_pa, PGSIZE,
		            PERM_R | PERM_W | PERM_X | PERM_U) < 0) {
			pmem_free_user(new_pa);
			return -1;
		}
	}
	return 0;
}
static void forkret(void)
{
	proc_t *p = myproc();
	arch_mmu_activate(p->pgtbl);
	arch_set_kernel_stack(p->kstack + KSTACK_SIZE);
	trap_return_to_user(p->tf);
}

proc_t *proc_copy(void)
{
}
