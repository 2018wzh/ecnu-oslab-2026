// 缓冲区缓存 (buffer cache)。
// 解决: 磁盘 I/O 慢 (毫秒级 vs 内存纳秒级)、同块被多次读取、以及多处并发修改
// 同块的一致性。设计: 固定 NBUF 的 LRU 双向链表 + 引用计数 —— 命中移到表头,
// 未命中从表尾找一个 refcnt==0 的复用。必须用睡眠锁: 磁盘读可能耗时几毫秒,
// 自旋会让 CPU 空转。判断标准: 持锁期间做 I/O -> 睡眠锁, 短临界区 -> 自旋锁。
#include <kernel/types.h>
#include <kernel/fs.h>
#include <kernel/print.h>
#include <kernel/string.h>
#include <kernel/sync.h>
#include <kernel/block.h>
#include <kernel/arch.h>

#define NBUF 30

static struct {
	spinlock_t lk;
	buf_t buf[NBUF];
	/* 双向链表: head.next 是最最近使用的, head.prev 是最久未使用的 */
	buf_t head;
} bcache;

void bio_init(void)
{
	spinlock_init(&bcache.lk, "bcache");
	bcache.head.prev = &bcache.head;
	bcache.head.next = &bcache.head;
	for (int i = 0; i < NBUF; i++) {
		buf_t *b = &bcache.buf[i];
		b->blockno = 0;
		b->refcnt = 0;
		b->valid = 0;
		sleeplock_init(&b->lock, "buffer");

		/* 头插法: 依次插入, 最终顺序是 buf[NBUF-1] ... buf[0] */
		b->next = bcache.head.next;
		b->prev = &bcache.head;
		bcache.head.next->prev = b;
		bcache.head.next = b;
	}
	/* 这一行与 [fs] 超级块那行说的是同一件事的两个细节, 删掉 ——
	 * 缓冲区数量在代码里是常量 (NBUF), 不需要每次启动都打印。
	 * 需要时看 bio.c 顶部的说明。 */
}
static void buf_move_to_front(buf_t *b)
{
	if (b->prev && b->next) {
		/* 已在链表中: 先摘除 */
		b->prev->next = b->next;
		b->next->prev = b->prev;
	}

	/* 插入到 head 之后 (链表头部 = 最近使用) */
	b->next = bcache.head.next;
	b->prev = &bcache.head;
	bcache.head.next->prev = b;
	bcache.head.next = b;
}
buf_t *bread(uint32 blockno)
{
}
void bwrite(buf_t *b)
{
	if (!b->valid)
		panic("bwrite: 试图写一个无效的 buf");

	if (block_write(b->blockno, b->data, 1) < 0)
		panic("bwrite: 写入块 %u 失败", b->blockno);
}
void brelse(buf_t *b)
{
	if (!spinlock_holding(&bcache.lk))
		;   /* 这里不需要持有 bcache.lk, 只是防御性检查 */
	if (b->refcnt < 1)
		panic("brelse: 引用计数错误 (blockno=%u)", b->blockno);

	sleeplock_release(&b->lock);

	spinlock_acquire(&bcache.lk);
	b->refcnt--;
	/* 只把"还有人用"的 buf 移到链表头。
	 * refcnt 为 0 的 buf 应当留在链表尾部附近, 这样下次
	 * 需要复用空间时会优先选中它 —— 这是 LRU 策略的关键。 */
	if (b->refcnt > 0)
		buf_move_to_front(b);
	spinlock_release(&bcache.lk);
}
