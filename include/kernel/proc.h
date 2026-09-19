/* 进程管理接口 (generic kernel): struct proc 里的 ctx 字段类型是 context_t,
 * 进程逻辑 generic, 上下文布局交给 arch 层定义, 换架构不必改 struct proc。 */
#ifndef __KERNEL_PROC_H__
#define __KERNEL_PROC_H__

#include <kernel/types.h>
#include <kernel/sync.h>
#include <kernel/mm.h>
#include <kernel/arch_context.h>
#include <kernel/arch_trap.h>
#include <kernel/fs.h>

#define NPROC      64          /* 最大进程数 */
#define PROC_NAME_MAX 16       /* 进程名长度 */
#define KSTACK_SIZE PGSIZE
#define N_CPU_MAX  8

/* 进程状态 */
enum proc_state {
	PROC_UNUSED = 0,   /* 未使用 (可分配) */
	PROC_RUNNABLE,     /* 就绪, 可以被调度 */
	PROC_RUNNING,      /* 正在某个 CPU 上运行 */
	PROC_SLEEPING,     /* 睡眠等待某个事件 */
	PROC_ZOMBIE,       /* 已退出, 等待父进程回收 */
};

typedef struct proc {
	/* ---- 调度与状态 ---- */
	enum proc_state state;
	int pid;
	int cpuid;                  /* 正在哪个 CPU 上运行; -1 表示没在运行 */
	char name[PROC_NAME_MAX];
	struct proc *parent;
	spinlock_t lk;              /* 保护本结构体的字段 */

	/* ---- 架构相关 ---- */
	context_t ctx;    /* 内核上下文 (切换时保存/恢复) */
	trapframe_t *tf;       /* 用户态现场 (位于内核栈顶端) */
	uint64 kstack;              /* 内核栈底部物理地址 */
	uint64 sz;                  /* 用户内存大小 (字节) */

	/* ---- 地址空间 ---- */
	pgtbl_t pgtbl;              /* 用户页表 */
	uint64 heap_top;            /* 用户堆顶 */
	uint64 ustack_npage;        /* 用户栈页数 */
	mmap_region_t *mmap;        /* mmap 区域链表头 */

/* 进程管理接口 (generic kernel): struct proc 里的 ctx 字段类型是 context_t,
 * 进程逻辑 generic, 上下文布局交给 arch 层定义, 换架构不必改 struct proc。 */
	struct file *ofile[NOFILE];

	/* ---- 待 lab-6/9 使用 ---- */
	uint64 chan;                /* 睡眠等待的通道 (类似 xv6 的做法) */
	int killed;                 /* 是否被要求退出 */
	uint64 exit_status;         /* 退出码, 供父进程读取 */
} proc_t;

/* --------------------------------------------------------------------------
 * 进程管理
 * -------------------------------------------------------------------------- */

/* 初始化进程表 */
void proc_init(void);

/* 创建第一个用户进程 (从内核嵌入的 initcode 启动) */
void proc_make_first(void);

/* 分配一个空闲的进程槽位 (未初始化) */
proc_t *proc_alloc(void);

/* 复制当前进程, 用于 fork */
proc_t *proc_copy(void);

/* 加载内存中的 ELF 到进程 p 的地址空间, 返回 0 成功 */
int exec_load(proc_t *p, const uint8 *elf, uint64 elf_len);

/* 加载"扁平二进制"镜像 (无 ELF 头, 用于内核内嵌的 initcode)。
 * perm 应包含 PERM_U, 否则用户态无法访问自己的代码。 */
int exec_load_flat(proc_t *p, const uint8 *img, uint64 len, uint64 vaddr,
                   int perm, uint64 stack_top, uint64 entry);

/* 切换到进程 p 的地址空间并进入用户态 (不返回) */
void user_enter(proc_t *p);

/* 从 trap 返回用户态: 参数是用户态现场 (trapframe), 不返回。
 * 实现在 arch/<arch>/trap/entry.S —— "怎么返回用户态" 是架构的事实。
 * 调用点有两处: trap 处理完之后的返回, 以及新进程第一次被调度时的
 * 入口 (见 proc.c 的 forkret)。 */
void trap_return_to_user(trapframe_t *tf);

/* 让出 CPU (被时钟中断调用, 也可主动调用) */
void proc_yield(void);
void proc_sleep(void *chan, spinlock_t *lk);
spinlock_t *proc_wait_lock(void);

/* 唤醒所有在 chan 上睡眠的进程 */
void proc_wakeup(void *chan);

/* 当前进程 / 指定 CPU 上的进程 */
proc_t *myproc(void);
proc_t *proc_of_cpu(int cpuid);

/* 退出当前进程 */
void proc_exit(int status);

/* 回收子进程 */
void proc_free(proc_t *p);

/* 打印所有进程状态 (调试用) */
void proc_dump(void);

/* 暴露进程表 (供调度器扫描; 返回的数组长度固定为 NPROC) */
proc_t *proc_table_base(void);

/* 设置/更新当前 CPU 上运行的进程 */
void proc_set_current(proc_t *p);
void sched_init_hart(void);

// 切换到下一个就绪进程。调用前不得持有任何自旋锁 (否则可能死锁);
// "返回两次": 本进程重新被调度时从调用点之后继续; 无就绪进程时可能切到 idle。

/* 进程管理接口 (generic kernel): struct proc 里的 ctx 字段类型是 context_t,
 * 进程逻辑 generic, 上下文布局交给 arch 层定义, 换架构不必改 struct proc。 */
void sched_switch(void);

#endif /* __KERNEL_PROC_H__ */
