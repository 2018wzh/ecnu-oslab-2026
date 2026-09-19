// 磁盘块分配与回收, 用位图记录数据块占用: 第 N 位=1 表示第 N 块被占用。
// 位图从块 1 开始, 512 字节=4096 位可描述 4096 块 (fs_init 校验 FSSIZE 不超限)。
// 靠 bread/brelse 走缓存, 位图块几乎总在缓存里, 所以分配释放是内存操作。
#include <kernel/types.h>
#include <kernel/fs.h>
#include <kernel/print.h>
#include <kernel/string.h>

static superblock_t sb;

superblock_t *fs_superblock(void)
{
	return &sb;
}

/* --------------------------------------------------------------------------
 * 从磁盘读取超级块并校验
 * -------------------------------------------------------------------------- */
int fs_read_superblock(void)
{
	buf_t *b = bread(0);
	/* 直接从 buf 里读字段。注意 mkfs 是按小端逐字节写入的,
	 * 而 RISC-V 也是小端, 所以这里可以直接按结构体解释。
	 * 如果将来支持大端平台, 这里需要改成逐字节解析。 */
	sb.magic      = *(uint32 *)(b->data + 0);
	sb.size       = *(uint32 *)(b->data + 4);
	sb.ninodes    = *(uint32 *)(b->data + 8);
	sb.inodestart = *(uint32 *)(b->data + 12);
	sb.nfiles     = *(uint32 *)(b->data + 20);
	brelse(b);

	if (sb.magic != FSMAGIC) {
		printf("[fs] 超级块魔数错误: 0x%x (期望 0x%x)\n",
		       sb.magic, FSMAGIC);
		printf("[fs] 说明磁盘镜像不是用配套的 mkfs 生成的, 或者未初始化\n");
		return -1;
	}

	/* 校验位图容量: 一个块只能描述 8*BSIZE 个块 */
	if (sb.size > 8 * BSIZE) {
		printf("[fs] 磁盘太大 (%u 块), 超出单块位图容量 (%d 块)\n",
		       sb.size, 8 * BSIZE);
		return -1;
	}

	printf("[fs] 超级块: %u 块 (%u KB), %u 个 inode\n",
	       sb.size, sb.size * BSIZE / 1024, sb.ninodes);
	return 0;
}
static uint32 data_start_block(void)
{
	uint32 inode_blocks = (sb.ninodes * sizeof(dinode_t) + BSIZE - 1) / BSIZE;
	return 2 + inode_blocks;
}

uint32 balloc(void)
{
}

void bfree(uint32 blockno)
{
}
