// 同步原语 (generic kernel)。
#ifndef __KERNEL_SYNC_H__
#define __KERNEL_SYNC_H__

#include <kernel/types.h>

// 自旋锁: locked 是否持有, cpuid 持有者 CPU 号, intr_save 保存的中断状态, name 锁名。
typedef struct spinlock {
	volatile uint32 locked;
	int cpuid;
	uint64 intr_save;
	const char *name;
} spinlock_t;

void spinlock_init(spinlock_t *lk, const char *name);
void spinlock_acquire(spinlock_t *lk);
void spinlock_release(spinlock_t *lk);
int  spinlock_holding(spinlock_t *lk);

// 睡眠锁: 允许在等待时让出 CPU (lab-6)。
typedef struct sleeplock {
	uint32 locked;
	spinlock_t lk;
	int pid;
	const char *name;
} sleeplock_t;

void sleeplock_init(sleeplock_t *lk, const char *name);
void sleeplock_acquire(sleeplock_t *lk);
void sleeplock_release(sleeplock_t *lk);
int  sleeplock_holding(sleeplock_t *lk);

#endif /* __KERNEL_SYNC_H__ */
