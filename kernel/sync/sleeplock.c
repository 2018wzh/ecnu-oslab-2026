// 睡眠锁 (generic kernel)。
// 自旋锁等不到就空转占 CPU, 适合极短临界区, 持锁期间不能睡眠 (否则死锁);
// 睡眠锁等不到就让出 CPU, 适合可能很长、尤其含磁盘 I/O 的临界区 (磁盘读要睡眠
// 等中断, 必须允许睡眠的锁; 用自旋锁会空转几毫秒浪费一个核)。
// 实现: 复用 proc_sleep/wakeup 通道, 每个睡眠锁用自己的地址当通道。
#include <kernel/types.h>
#include <kernel/sync.h>
#include <kernel/proc.h>
#include <kernel/print.h>

void sleeplock_init(sleeplock_t *lk, const char *name)
{
	spinlock_init(&lk->lk, "sleeplock");
	lk->locked = 0;
	lk->pid = 0;
	lk->name = name;
}

void sleeplock_acquire(sleeplock_t *lk)
{
	spinlock_acquire(&lk->lk);

	// 用 while 而非 if: 被唤醒后必须重新检查。睡眠可能被非预期唤醒 (spurious),
	// 或我们被唤醒到真正拿锁间又有进程抢先。用 if 会让两进程同时认为持有锁。
	while (lk->locked) {
		proc_sleep(lk, &lk->lk);
	}

	lk->locked = 1;
	lk->pid = myproc() ? myproc()->pid : 0;
	spinlock_release(&lk->lk);
}

void sleeplock_release(sleeplock_t *lk)
{
	spinlock_acquire(&lk->lk);

	if (!lk->locked)
		panic("sleeplock_release: 锁 %s 未被持有", lk->name ? lk->name : "?");

	lk->locked = 0;
	lk->pid = 0;

	// 唤醒所有等待者 (无等待队列, 无法精确指定唤谁)。等待者会在 while 循环
	// 重新竞争, 只有一个能拿到锁, 其余继续睡—— 这叫"惊群", 有性能损耗但
	// 实现简单; 真实内核用等待队列避免 (可作扩展练习)。
	proc_wakeup(lk);

	spinlock_release(&lk->lk);
}

int sleeplock_holding(sleeplock_t *lk)
{
	int r;
	spinlock_acquire(&lk->lk);
	r = lk->locked && lk->pid == (myproc() ? myproc()->pid : -1);
	spinlock_release(&lk->lk);
	return r;
}
