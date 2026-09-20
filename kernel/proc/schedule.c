#include <kernel/proc.h>
#include <kernel/print.h>
#include <platform.h>
context_t scheduler_context[NCPU];
// TODO(lab-6): 开中断扫描各槽；持目标锁选择 RUNNABLE，置 RUNNING/current 后持锁切换。
// 返回后仍持该锁，清 current、释放锁。调度器只访问本核 context。
void proc_scheduler(void) { panic("TODO(lab-6): proc_scheduler"); }
// TODO(lab-6): 当前进程锁是唯一持有的锁，关中断且状态不为 RUNNING；持锁切回调度器。
// 用 arch_interrupt_depth 检查唯一锁；切换前保存调用者释放锁后的中断策略，恢复后
// 经 arch_set_resume_interrupts 设回该策略，不复制 depth/owner；保证 trap 继续保持关中断。
// 返回时由当前 CPU 的调度器交回同一进程锁；不能缓存切换前的 CPU 指针。
void proc_sched(void) { panic("TODO(lab-6): proc_sched"); }
// TODO(lab-6): 获取自身锁，RUNNING -> RUNNABLE，持锁 sched，恢复后解锁。
void proc_yield(void) { panic("TODO(lab-6): proc_yield"); }
// TODO(lab-6): 先取进程锁再释放条件锁；若两锁相同不重复获取。
// 发布 chan/SLEEPING 后持进程锁 sched；恢复后清 chan，交还到原条件锁，返回时仍持条件锁。
void proc_sleep(void *chan, spinlock_t *condition_lock)
{ (void)chan; (void)condition_lock; panic("TODO(lab-6): proc_sleep"); }
// TODO(lab-6): 逐槽持锁唤醒匹配 chan 的 SLEEPING 进程，跳过自身；持条件锁改变条件再唤醒。
void proc_wakeup(void *chan) { (void)chan; panic("TODO(lab-6): proc_wakeup"); }
// TODO(lab-6): 首次在自己的内核栈执行，释放调度器交来的进程锁，再 enter_user。
void proc_first_return(void) { panic("TODO(lab-6): proc_first_return"); }

// TODO(lab-7): proc_first_return 中，proczero 释放进程锁后、enter_user 前单次 fs_init；可睡眠，失败停止。

// TODO(lab-9): 首进程 fs 初始化后调用 files 初始化任务，cwd=root，依次打开 stdin/stdout/stderr。
