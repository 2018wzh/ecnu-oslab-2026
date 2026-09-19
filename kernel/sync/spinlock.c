// 自旋锁 (generic kernel)。
// 关中断与加锁解决不同问题: 自旋锁防其他 CPU 同时进临界区, 关中断防本 CPU 被
// 中断打断后中断处理程序又去抢同一把锁 (自等自 -> 死锁)。所以持自旋锁期间
// 通常要关中断, 两者配合。释放时恢复"加锁前保存的状态"而非无条件开中断,
// 否则嵌套临界区会被破坏。
#include <kernel/types.h>
#include <kernel/sync.h>
#include <kernel/arch.h>

void spinlock_init(spinlock_t *lk, const char *name)
{
        lk->locked = 0;
        lk->name = name;
        lk->cpuid = -1;
}

void spinlock_acquire(spinlock_t *lk)
{
}

void spinlock_release(spinlock_t *lk)
{
}

int spinlock_holding(spinlock_t *lk)
{
        int r = (lk->locked && lk->cpuid == arch_cpu_id());
        return r;
}
