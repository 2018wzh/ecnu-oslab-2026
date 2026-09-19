// inode 层 (文件元数据与数据块访问)。
// inode 把一个文件表示为: 元数据 (类型/大小/链接数) + 数据块列表。
// 块寻址: 直接块 addrs[0..11] (6KB) + 一级间接块 addrs[12] (存 128 个块号=64KB),
// 单文件最大 70KB。dinode 必须定长才能按 inode 号直接定位; 更大文件用多级间接块。
// inode 读写会触发磁盘 I/O, 所以必须用睡眠锁。
#include <kernel/types.h>
#include <kernel/fs.h>
#include <kernel/print.h>
#include <kernel/string.h>
#include <kernel/sync.h>
#include <kernel/mm.h>
#include <kernel/proc.h>

/* inode 缓存: 内存中的 inode 表 */
static struct {
	spinlock_t lk;
	inode_t inode[NINODES];
} itable;
static uint32 inode_index(uint32 inum)
{
	if (inum == 0)
		panic("inode_index: inode 号 0 是保留值, 表示无效");
	return inum - 1;
}

static dinode_t *dinode_at(buf_t *b, uint32 inum)
{
	uint32 per_block = BSIZE / sizeof(dinode_t);
	return (dinode_t *)b->data + (inode_index(inum) % per_block);
}

/* 计算 inum 对应的 inode 所在块号 */
static uint32 inode_block(uint32 inum)
{
	uint32 per_block = BSIZE / sizeof(dinode_t);
	uint32 inodestart = fs_superblock()->inodestart;
	return inodestart + (inode_index(inum) / per_block);
}
static void inode_load(inode_t *ip)
{
	if (ip->ref < 1)
		panic("inode_load: 引用计数为 0");

	uint32 blk = inode_block(ip->inum);
	buf_t *b = bread(blk);
	dinode_t *dip = dinode_at(b, ip->inum);

	ip->type  = dip->type;
	ip->major = dip->major;
	ip->minor = dip->minor;
	ip->nlink = dip->nlink;
	ip->size  = dip->size;
	memmove(ip->addrs, dip->addrs, sizeof(ip->addrs));

	brelse(b);
	ip->valid = 1;
}

/* --------------------------------------------------------------------------
 * 把内存中的 inode 写回磁盘
 * -------------------------------------------------------------------------- */
void inode_update(inode_t *ip)
{
	if (ip->ref < 1)
		panic("inode_update: 引用计数为 0");
	if (!sleeplock_holding(&ip->lock))
		panic("inode_update: 未持有 inode 锁 (inum=%u)", ip->inum);

	uint32 blk = inode_block(ip->inum);
	buf_t *b = bread(blk);
	dinode_t *dip = dinode_at(b, ip->inum);

	dip->type  = ip->type;
	dip->major = ip->major;
	dip->minor = ip->minor;
	dip->nlink = ip->nlink;
	dip->size  = ip->size;
	memmove(dip->addrs, ip->addrs, sizeof(dip->addrs));

	bwrite(b);
	brelse(b);
}

void inode_init(void)
{
	spinlock_init(&itable.lk, "itable");
	for (int i = 0; i < NINODES; i++) {
		itable.inode[i].ref = 0;
		itable.inode[i].inum = 0;
		sleeplock_init(&itable.inode[i].lock, "inode");
	}
	printf("[fs] inode 缓存已初始化 (%d 项)\n", NINODES);
}
inode_t *inode_get(uint32 inum)
{
	spinlock_acquire(&itable.lk);

	/* 先在缓存中找 */
	for (int i = 0; i < NINODES; i++) {
		inode_t *ip = &itable.inode[i];
		if (ip->ref > 0 && ip->inum == inum) {
			ip->ref++;
			spinlock_release(&itable.lk);
			if (!ip->valid)
				inode_load(ip);
			return ip;
		}
	}

	/* 没找到: 分配一个空闲槽位 */
	for (int i = 0; i < NINODES; i++) {
		inode_t *ip = &itable.inode[i];
		if (ip->ref == 0) {
			ip->inum = inum;
			ip->ref = 1;
			ip->valid = 0;
			spinlock_release(&itable.lk);
			inode_load(ip);
			return ip;
		}
	}

	spinlock_release(&itable.lk);
	panic("inode_get: inode 缓存耗尽 (NINODES=%d 太小)", NINODES);
}
void inode_put(inode_t *ip)
{
	spinlock_acquire(&itable.lk);

	if (--ip->ref == 0 && ip->valid && ip->nlink == 0) {
		/* 需要真正回收: 必须先拿锁, 但拿锁可能睡眠,
		 * 所以要先释放 itable.lk (否则在持自旋锁时睡眠 = 死锁) */
		spinlock_release(&itable.lk);

		sleeplock_acquire(&ip->lock);

		/* 重新检查: 在我们拿锁期间可能有别人又引用了它 */
		if (ip->ref == 0 && ip->valid && ip->nlink == 0) {
			/* 释放所有数据块 */
			for (int i = 0; i < NDIRECT; i++) {
				if (ip->addrs[i]) {
					bfree(ip->addrs[i]);
					ip->addrs[i] = 0;
				}
			}
			/* 释放一级间接块及其指向的数据块 */
			if (ip->addrs[NDIRECT]) {
				buf_t *b = bread(ip->addrs[NDIRECT]);
				uint32 *a = (uint32 *)b->data;
				for (uint32 j = 0; j < NINDIRECT; j++) {
					if (a[j])
						bfree(a[j]);
				}
				brelse(b);
				bfree(ip->addrs[NDIRECT]);
				ip->addrs[NDIRECT] = 0;
			}

			ip->type = 0;
			inode_update(ip);
			ip->valid = 0;
		}

		sleeplock_release(&ip->lock);
		return;
	}

	spinlock_release(&itable.lk);
}

/* --------------------------------------------------------------------------
 * 取得 inode 的第 n 个数据块号; 若不存在且 alloc 为真则分配
 * -------------------------------------------------------------------------- */

static uint32 inode_block_map(inode_t *ip, uint32 bn, int alloc)
{
}
int inode_read(inode_t *ip, uint64 dst, uint32 off, uint32 n, int is_user)
{
}

/* --------------------------------------------------------------------------
 * 写入 inode 数据
 * -------------------------------------------------------------------------- */
int inode_write(inode_t *ip, uint64 src, uint32 off, uint32 n, int is_user)
{
}

/* --------------------------------------------------------------------------
 * 打印 inode 信息 (调试)
 * -------------------------------------------------------------------------- */
void inode_stat(inode_t *ip)
{
	static const char *types[] = { "free", "dir", "file", "dev" };
	printf("inode %u: type=%s size=%u nlink=%u\n",
	       ip->inum, types[ip->type <= 3 ? ip->type : 0], ip->size, ip->nlink);
}
