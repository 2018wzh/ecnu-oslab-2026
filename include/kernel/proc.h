#ifndef OSLAB_PROC_H
#define OSLAB_PROC_H
#include <kernel/mem.h>
#include <kernel/trap.h>
#include <kernel/context.h>
#include <kernel/lock.h>
#define N_PROC 32
#define USER_ENTRY 0x1000UL
#define TRAMPOLINE (VA_MAX - PAGE_SIZE)
#define TRAPFRAME (TRAMPOLINE - PAGE_SIZE)
#define USER_STACK_TOP TRAPFRAME
/* 每个内核栈一页，栈之间保留一页不映射；id 是 0..N_PROC 的槽索引，不是 PID。 */
#define KSTACK(id) (TRAPFRAME - ((uint64)(id) + 1) * 2 * PAGE_SIZE)
typedef struct {
    trapframe_t regs;
    uint64 kernel_satp, kernel_sp, kernel_entry, kernel_hart;
} user_frame_t;
_Static_assert(offsetof(user_frame_t, kernel_satp) == 272, "user frame layout");
_Static_assert(offsetof(user_frame_t, kernel_sp) == 280, "kernel sp layout");
_Static_assert(offsetof(user_frame_t, kernel_entry) == 288, "kernel entry layout");
_Static_assert(offsetof(user_frame_t, kernel_hart) == 296, "kernel hart layout");
_Static_assert(sizeof(user_frame_t) == 304, "user frame size");
typedef enum { UNUSED, ZOMBIE, SLEEPING, RUNNABLE, RUNNING } proc_state_t;
typedef struct proc {
    spinlock_t lock; /* 保护共享状态，跨切换由调度器和进程交接。 */
    char name[16];
    _Atomic(struct proc *) parent; /* 候选关系预筛可原子读取；修改仍须关系锁序。 */
    int exit_code;
    void *chan;
    int pid;
    proc_state_t state;
    pgtbl_t pgtbl;        /* 用户页表根的物理地址，内核恒等映射可访问。 */
    uint64 heap_top;      /* 字节单位；初值 0x2000，按页伸缩。 */
    uint64 ustack_npage;  /* 用户栈页数，初值 1。 */
    user_frame_t *frame;  /* 内核通过物理恒等映射访问独占 frame 页。 */
    uint64 kstack;        /* 高地址虚拟栈底，不是可直接回收的物理地址。 */
    struct mmap_region *mmap; /* 已分配区域链，首进程初始化为空。 */
    context_t context;
} proc_t;
extern proc_t proc_table[N_PROC];
extern proc_t *proczero;
int proc_alloc_pid(void);
void proc_pid_init(void);
void proc_table_init(void);
proc_t *proc_slot_alloc(void); /* 返回持锁槽，耗尽 NULL。 */
void proc_free(proc_t *p); /* 调用者持 p->lock；释放资源后仍持锁。 */
long proc_fork(void);
void proc_reparent(proc_t *parent);
void proc_try_wakeup(proc_t *p); /* 调用者持 p->lock。 */
void proc_exit(int status) __attribute__((noreturn));
long proc_wait(uint64 status);
void proc_scheduler(void) __attribute__((noreturn));
void proc_sched(void);
void proc_yield(void);
void proc_sleep(void *chan, spinlock_t *condition_lock);
void proc_wakeup(void *chan);
void proc_first_return(void) __attribute__((noreturn));
proc_t *myproc(void);
void set_current(proc_t *p);
/* frame_pa 是已清零的内核池页面；返回内核池中已初始化的页表。 */
pgtbl_t proc_pgtbl_init(uint64 frame_pa);
void proc_make_first(void);
void enter_user(void) __attribute__((noreturn));
void user_trap(void) __attribute__((noreturn));
extern char trampoline[], user_vector[], user_return[];
extern uint8 user_image_start[], user_image_end[];
#endif
