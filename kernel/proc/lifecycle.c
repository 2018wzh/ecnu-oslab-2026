#include <kernel/proc.h>
#include <kernel/print.h>
proc_t proc_table[N_PROC];
static spinlock_t pid_lock;
static uint32 next_pid = 1;
/* 教师 PID 辅助；独立锁串行分配，使用无符号计数避免有符号溢出。 */
int proc_alloc_pid(void)
{
    spinlock_acquire(&pid_lock);
    if (next_pid > 0x7fffffffU) {
        printf("PID exhausted: next=%p\n", (void *)(uint64)next_pid);
        panic("PID exhausted");
    }
    int pid = (int)next_pid++;
    spinlock_release(&pid_lock);
    return pid;
}
// TODO(lab-6): 主核发布前初始化 32 槽及各自锁，proczero=NULL；调用下面 PID 初始化外围。
void proc_table_init(void) { panic("TODO(lab-6): proc_table_init"); }
/* 教师初始化外围：只能在启动期间、没有并发分配时调用。 */
void proc_pid_init(void) { spinlock_init(&pid_lock, "pid"); next_pid = 1; }
// TODO(lab-6): 扫描并锁住 UNUSED 且 !reclaiming 的槽，持锁完成通用初始化；无空槽 NULL。
// 分配 pid、内核池清零的独立 frame/页表，kstack 按数组索引选择；context.ra=proc_first_return，sp=对齐栈顶。
// 返回仍持锁；发布 RUNNABLE 前保持不可调度，不引入第六种状态。
proc_t *proc_slot_alloc(void) { panic("TODO(lab-6): proc_slot_alloc"); }
// TODO(lab-6): 持 p->lock，且 p 已停止运行；uvm_destroy 释放 frame 一次，清 frame 指针。
// 回收 mmap 节点和普通资源，保留常驻内核栈及锁，最终 UNUSED；返回仍持锁。
void proc_free(proc_t *p) { (void)p; panic("TODO(lab-6): proc_free"); }
// TODO(lab-6): 无槽返回 -1；组合 uvm_copy_pgtbl 深复制普通页，独立复制 mmap 节点、frame 和进程字段。
// trampoline 共享，kstack 随槽复用；底层耗尽 panic，不要求回滚。
// 子 frame 复制父 ecall 时的状态，arch_syscall_return(...,0) 恰好推进子 PC 一次；
// 父返回子 pid，由用户 trap 的架构返回接口推进父 PC 一次。设置 parent 后发布 RUNNABLE、解锁。
long proc_fork(void) { panic("TODO(lab-6): proc_fork"); }
// TODO(lab-6): 将 parent 的所有孩子托孤给 proczero；按 README 父子锁序修改关系，唤醒需要回收的根。
void proc_reparent(proc_t *parent) { (void)parent; panic("TODO(lab-6): proc_reparent"); }
// TODO(lab-6): 调用者持 p->lock；只把等待自身通道的 SLEEPING 父进程置 RUNNABLE。
void proc_try_wakeup(proc_t *p) { (void)p; panic("TODO(lab-6): proc_try_wakeup"); }
// TODO(lab-6): 禁止 proczero 退出；reparent、记录 exit_code、唤醒父亲，置 ZOMBIE、释放父锁后仅持自身锁 sched。
void proc_exit(int status) { (void)status; panic("TODO(lab-6): proc_exit"); }
// TODO(lab-6): 持父锁原子预筛 parent，只对匹配的孩子取锁并复核；无子 -1；找到 ZOMBIE，status!=0 才复制 i32 状态。
// 先复制再回收并返回 pid；非法复制沿用 lab-5 panic，不要求坏指针重试。无已退出孩子则睡眠。
long proc_wait(uint64 status) { (void)status; panic("TODO(lab-6): proc_wait"); }

// TODO(lab-9): init/slot_alloc 初始化 files/cwd 为空和 reclaiming=false；选槽必须同时满足 UNUSED/Unused 且 !reclaiming。
// fork 增加所有 file/cwd 引用；首进程 first_return 解锁并 fs_init 后设置 cwd=root 和 fd0/1/2。
// proc_free/free 两阶段：只持目标进程锁，标记 reclaiming 并移出 files/cwd；释放该锁，关闭移出的引用；
// 关闭后如需清除已发布 parent 关系，先取父锁再取目标锁；未发布槽只取目标锁。
// 完成其余资源回收、清 parent、置 UNUSED/Unused 并清标记；释放父锁，返回仍持目标锁。不能在锁内关闭/Drop。
// wait 持父/子锁复核 Zombie 且 !reclaiming，复制退出码并记住 pid；释放父锁后调用 free；
// 其他等待者跳过 reclaiming 槽且不得视其为无子；完成后唤醒等待父进程，遵守父->子锁序。
// 分配失败清理也只持目标锁使用同一协议；其他分配者不能复用回收中的槽。
// exit 只发布 Zombie；文件/cwd 的释放在 free 阶段，不提前在 exit 关闭。
