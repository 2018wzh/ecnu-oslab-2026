#include <kernel/lock.h>
#include <kernel/print.h>
// TODO(lab-6): 初始化内部条件锁及持有状态。
void sleeplock_init(sleeplock_t *lk) { (void)lk; panic("TODO(lab-6): sleeplock_init"); }
// TODO(lab-6): 在内部锁下判断当前进程持有情况。
bool sleeplock_holding(sleeplock_t *lk) { (void)lk; panic("TODO(lab-6): sleeplock_holding"); }
// TODO(lab-6): 循环检查条件，通过 proc_sleep 原子等待，不忙等。
void sleeplock_acquire(sleeplock_t *lk) { (void)lk; panic("TODO(lab-6): sleeplock_acquire"); }
// TODO(lab-6): 检查所有者，清持有状态并唤醒等待者。
void sleeplock_release(sleeplock_t *lk) { (void)lk; panic("TODO(lab-6): sleeplock_release"); }
