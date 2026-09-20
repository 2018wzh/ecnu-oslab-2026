#ifndef OSLAB_LOCK_H
#define OSLAB_LOCK_H
#include <kernel/types.h>
typedef struct {
    uint32 locked; /* 锁是否持有；并发访问须使用原子操作。 */
    int owner; /* 持有者 cpuid，未持有时为 -1。 */
    const char *name; /* 锁名，用于诊断。 */
} spinlock_t;
void spinlock_init(spinlock_t *lk, const char *name);
bool spinlock_holding(spinlock_t *lk);
void spinlock_acquire(spinlock_t *lk);
void spinlock_release(spinlock_t *lk);
#endif
