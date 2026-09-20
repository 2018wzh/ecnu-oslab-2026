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
typedef struct { spinlock_t lock; bool locked; int pid; } sleeplock_t;
void sleeplock_init(sleeplock_t *lk);
bool sleeplock_holding(sleeplock_t *lk);
void sleeplock_acquire(sleeplock_t *lk);
void sleeplock_release(sleeplock_t *lk);
#endif
