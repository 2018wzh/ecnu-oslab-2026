#ifndef OSLAB_LOCK_H
#define OSLAB_LOCK_H
#include <kernel/types.h>
typedef struct {
    uint32 locked;
    int owner;
    const char *name;
} spinlock_t;
void spinlock_init(spinlock_t *lk, const char *name);
bool spinlock_holding(spinlock_t *lk);
void spinlock_acquire(spinlock_t *lk);
void spinlock_release(spinlock_t *lk);
#endif
