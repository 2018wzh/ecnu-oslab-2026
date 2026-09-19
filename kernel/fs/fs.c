// 文件系统初始化。
// 初始化顺序不能错, 每层依赖下面一层: 缓冲区缓存 -> 超级块 -> inode 缓存
// -> 文件对象表。顺序错了会读到垃圾数据 (如 inodestart 还是 0)。
#include <kernel/types.h>
#include <kernel/fs.h>
#include <kernel/print.h>
#include <kernel/block.h>

void inode_init(void);

static int fs_ready = 0;

int fs_mounted(void)
{
	return fs_ready;
}

void fs_init(void)
{
	printf("\n[fs] ===== 文件系统初始化 =====\n");

	/* 1. 缓冲区缓存 */
	bio_init();

	/* 2. 超级块。读失败说明磁盘镜像有问题, 不要继续往下走 ——
	 *    否则后续操作会在错误的参数上运行, 产生难以理解的行为。 */
	if (fs_read_superblock() < 0) {
		printf("[fs] 文件系统初始化失败 (超级块错误)\n");
		printf("[fs] 提示: 确认 QEMU 的 -drive 指向了 mkfs 生成的 disk.img\n");
		return;
	}

	/* 3. inode 缓存 */
	inode_init();


	// 4. 文件对象表
	inode_t *root = inode_get(ROOTINO);
	if (root->type != T_DIR) {
		printf("[fs] 警告: 根目录 inode 类型异常 (%u)\n", root->type);
		inode_put(root);
		return;
	}

	printf("[fs] 根目录 inode 正常 (inum=%u, size=%u)\n", root->inum, root->size);
	inode_put(root);

	fs_ready = 1;
	printf("[fs] ===== 文件系统就绪 =====\n\n");
}

/* --------------------------------------------------------------------------
 * 列出根目录内容 (调试用, 同时验证目录读取链路)
 * -------------------------------------------------------------------------- */
void fs_list_root(void)
{
	inode_t *root = inode_get(ROOTINO);
	if (root->type != T_DIR) {
		inode_put(root);
		return;
	}

	printf("[fs] 根目录内容:\n");
	uint32 off = 0;
	dirent_t de;
	while (off < root->size) {
		int r = inode_read(root, (uint64)&de, off, sizeof(de), 0);
		if (r != (int)sizeof(de))
			break;
		off += sizeof(de);
		if (de.inum == 0)
			continue;
		printf("       inum=%-4u %.14s\n", de.inum, de.name);
	}

	inode_put(root);
}
