#include <kernel/lock.h>
#include <kernel/arch.h>
#include <kernel/print.h>

// TODO(lab-1): 初始化未持有的锁；owner 为 -1。
void spinlock_init(spinlock_t *lk, const char *name)
{ (void)lk; (void)name; panic("TODO(lab-1): spinlock_init"); }
// TODO(lab-1): 判断当前 CPU 是否持有锁。
bool spinlock_holding(spinlock_t *lk)
{ (void)lk; panic("TODO(lab-1): spinlock_holding"); }
// TODO(lab-1): push_off 后原子获取锁，记录持有者。
void spinlock_acquire(spinlock_t *lk)
{ (void)lk; panic("TODO(lab-1): spinlock_acquire"); }
// TODO(lab-1): 检查持有者，发布受保护写入，解锁并 pop_off。
void spinlock_release(spinlock_t *lk)
{ (void)lk; panic("TODO(lab-1): spinlock_release"); }
