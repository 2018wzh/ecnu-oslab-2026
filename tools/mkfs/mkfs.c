// 文件系统镜像生成工具 (在开发机上运行)。
// 它是宿主程序不是内核: 用 HOSTCC (开发机 gcc) 编译, 把普通 RISC-V 可执行文件
// 打包成磁盘镜像。若用 riscv64-gcc 编译会得到 RISC-V 程序, 开发机加载不了
// ("Exec format error") —— 交叉编译环境经典错误。
// 磁盘格式是"契约"不是"内存结构体": 用显式 put16/put32 逐字节写字段, 而非直接
// fwrite 一个 struct (结构体布局受编译器/架构对齐与字节序影响, 一旦变就读不出了)。
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* --------------------------------------------------------------------------
 * 磁盘布局 (小端序)
 *
 *   块 0             : 超级块 (superblock)
 *   块 1             : 空闲位图起始 (每个 bit 表示一个块是否被占用)
 *   块 1 + bitmap    : inode 区
 *   之后             : 数据区 (存放文件内容与目录项)
 *
 * 块大小固定 512 字节。
 * -------------------------------------------------------------------------- */

#define BSIZE        512
#define FSMAGIC      0x10203040
#define NDIRECT      12
#define NINDIRECT    (BSIZE / 4)  /* 一级间接块能存的块号个数 */
#define MAXFILE      (NDIRECT + NINDIRECT)
#define MAXNAME      14
#define NINODES      200
#define ROOTINO      1

#define FSSIZE       2000   /* 镜像总块数 */
#define SUPERBLOCK_BLOCK  0
#define BITMAP_BLOCK      1
#define INODE_START_BLOCK 2

#define DINODE_SIZE   ((uint32_t)sizeof(struct dinode))
#define INODES_PER_BLOCK (BSIZE / DINODE_SIZE)
#define INODE_BLOCKS  ((NINODES + INODES_PER_BLOCK - 1) / INODES_PER_BLOCK)
#define DATA_START_BLOCK (INODE_START_BLOCK + INODE_BLOCKS)
struct dinode {
	uint16_t type;
	uint16_t major;
	uint16_t minor;
	uint16_t nlink;
	uint32_t size;
	uint32_t addrs[NDIRECT + 1];
} __attribute__((packed));

struct dirent {
	uint16_t inum;
	char name[MAXNAME];
};

/* ---- 字节序安全的写入辅助函数 ---- */
static void put16(uint8_t *p, uint16_t v)
{
	p[0] = v & 0xff;
	p[1] = (v >> 8) & 0xff;
}

static void put32(uint8_t *p, uint32_t v)
{
	p[0] = v & 0xff;
	p[1] = (v >> 8) & 0xff;
	p[2] = (v >> 16) & 0xff;
	p[3] = (v >> 24) & 0xff;
}

static uint8_t image[FSSIZE * BSIZE];
static uint8_t inode_used[NINODES];
static uint8_t block_used[FSSIZE];

/* 位图操作 */
static void bitmap_mark_used(uint32_t blk) { block_used[blk] = 1; }

/* 分配一个数据块 */
static uint32_t balloc(void)
{
	for (uint32_t i = DATA_START_BLOCK; i < FSSIZE; i++) {
		if (!block_used[i]) {
			bitmap_mark_used(i);
			memset(&image[i * BSIZE], 0, BSIZE);
			return i;
		}
	}
	fprintf(stderr, "mkfs: 磁盘空间不足\n");
	exit(1);
}

/* 分配一个 inode */
static uint16_t ialloc(uint16_t type)
{
	for (uint16_t i = 1; i < NINODES; i++) {
		if (!inode_used[i]) {
			inode_used[i] = 1;
			struct dinode *dip = (struct dinode *)&image[INODE_START_BLOCK * BSIZE + (i - 1) * sizeof(struct dinode)];
			memset(dip, 0, sizeof(*dip));
			dip->type = type;
			dip->nlink = 1;
			return i;
		}
	}
	fprintf(stderr, "mkfs: inode 用尽\n");
	exit(1);
}

/* 把 inode 写回镜像 (用显式字节写入, 保证布局确定) */
static void write_inode(uint16_t inum, struct dinode *d)
{
	uint8_t *p = &image[INODE_START_BLOCK * BSIZE + (inum - 1) * sizeof(struct dinode)];
	memset(p, 0, sizeof(struct dinode));
	put16(p + 0, d->type);
	put16(p + 2, d->major);
	put16(p + 4, d->minor);
	put16(p + 6, d->nlink);
	put32(p + 8, d->size);
	for (int i = 0; i < NDIRECT + 1; i++)
		put32(p + 12 + i * 4, d->addrs[i]);
}

/* 往 inode 里写数据 */
static void write_data(uint16_t inum, const void *data, uint32_t len)
{
	struct dinode d;
	uint8_t *p = &image[INODE_START_BLOCK * BSIZE + (inum - 1) * sizeof(struct dinode)];
	d.type = p[0];
	d.size = 0;
	memset(d.addrs, 0, sizeof(d.addrs));
	d.nlink = 1;
	d.major = d.minor = 0;

	uint32_t off = 0;
	const uint8_t *src = data;
	uint32_t *indirect = 0;   /* 一级间接块 (按需分配) */

	while (off < len) {
		uint32_t blk_idx = off / BSIZE;
		uint32_t b;

		if (blk_idx < NDIRECT) {
			b = balloc();
			d.addrs[blk_idx] = b;
		} else if (blk_idx < NDIRECT + NINDIRECT) {
			if (!indirect) {
				uint32_t ib = balloc();
				d.addrs[NDIRECT] = ib;
				indirect = (uint32_t *)&image[ib * BSIZE];
			}
			uint32_t i = blk_idx - NDIRECT;
			b = balloc();
			indirect[i] = b;
		} else {
			fprintf(stderr, "mkfs: 文件过大 (超过 MAXFILE=%d 块)\n",
				(int)MAXFILE);
			exit(1);
		}

		uint32_t n = BSIZE;
		if (len - off < n) n = len - off;
		memcpy(&image[b * BSIZE], src + off, n);
		off += n;
	}
	d.size = len;
	write_inode(inum, &d);
}

/* 在目录中增加一个条目 */
static void add_dirent(uint16_t dir_inum, uint16_t child_inum, const char *name)
{
	/* 简化处理: 直接取 inode 的第一个数据块当作目录块 */
	uint8_t *p = &image[INODE_START_BLOCK * BSIZE + (dir_inum - 1) * sizeof(struct dinode)];
	uint32_t blk0 = 0;
	for (int i = 0; i < NDIRECT; i++) {
		uint32_t v = p[12 + i * 4] | (p[13 + i * 4] << 8) |
		             (p[14 + i * 4] << 16) | ((uint32_t)p[15 + i * 4] << 24);
		if (v) { blk0 = v; break; }
	}
	if (!blk0) {
		blk0 = balloc();
		/* 写回 addrs[0] */
		p[12] = blk0 & 0xff; p[13] = (blk0 >> 8) & 0xff;
		p[14] = (blk0 >> 16) & 0xff; p[15] = (blk0 >> 24) & 0xff;
	}

	uint8_t *dir = &image[blk0 * BSIZE];
	for (int i = 0; i < BSIZE / (int)sizeof(struct dirent); i++) {
		uint8_t *slot = dir + i * sizeof(struct dirent);
		uint16_t cur = (uint16_t)(slot[0] | (slot[1] << 8));
		if (cur == 0) {
			/* 目录项布局: inum(2 字节小端) + name(14 字节, 补零结尾) */
			slot[0] = child_inum & 0xff;
			slot[1] = (child_inum >> 8) & 0xff;
			memset(slot + 2, 0, MAXNAME);
			size_t n = strlen(name);
			if (n > MAXNAME) n = MAXNAME;
			memcpy(slot + 2, name, n);
			return;
		}
	}
	fprintf(stderr, "mkfs: 目录已满\n");
	exit(1);
}

int main(int argc, char *argv[])
{
	if (argc < 3) {
		fprintf(stderr, "用法: %s <镜像文件> <可执行文件1> [可执行文件2 ...]\n", argv[0]);
		return 1;
	}

	memset(image, 0, sizeof(image));
	memset(inode_used, 0, sizeof(inode_used));
	memset(block_used, 0, sizeof(block_used));

	/* 超级块、位图、inode 区都标记为已用, 不能被数据分配占用 */
	for (uint32_t b = 0; b < DATA_START_BLOCK; b++)
		bitmap_mark_used(b);

	/* ---- 创建根目录 ---- */
	uint16_t root = ialloc(1 /* T_DIR */);
	if (root != ROOTINO) {
		fprintf(stderr, "mkfs: 根目录 inode 号不是 %d\n", ROOTINO);
		return 1;
	}
	write_data(root, "", 0);
	{
		/* 分配并清零根目录的第一个数据块 */
		uint32_t b = balloc();
		struct dinode d;
		memset(&d, 0, sizeof(d));
		d.type = 1;        /* T_DIR */
		d.nlink = 1;
		d.size = BSIZE;    /* 一个块, 全零表示所有目录项都空闲 */
		d.addrs[0] = b;
		write_inode(root, &d);
	}

	/* ---- 把每个可执行文件写入镜像 ---- */
	for (int i = 2; i < argc; i++) {
		FILE *f = fopen(argv[i], "rb");
		if (!f) {
			fprintf(stderr, "mkfs: 无法打开 %s\n", argv[i]);
			return 1;
		}
		fseek(f, 0, SEEK_END);
		long sz = ftell(f);
		fseek(f, 0, SEEK_SET);

		uint8_t *buf = malloc(sz);
		if (fread(buf, 1, sz, f) != (size_t)sz) {
			fprintf(stderr, "mkfs: 读取 %s 失败\n", argv[i]);
			return 1;
		}
		fclose(f);

		uint16_t inum = ialloc(2 /* T_FILE */);
		write_data(inum, buf, (uint32_t)sz);
		free(buf);

		/* 取路径的文件名部分作为目录项名字 */
		const char *base = strrchr(argv[i], '/');
		base = base ? base + 1 : argv[i];
		char name[MAXNAME + 1];
		strncpy(name, base, MAXNAME);
		name[MAXNAME] = '\0';
		/* 去掉 .elf 后缀, 保持名字简短 */
		char *dot = strstr(name, ".elf");
		if (dot) *dot = '\0';

		add_dirent(root, inum, name);
		printf("mkfs: %-12s -> inode %d (%ld 字节)\n", name, inum, sz);
	}

	/* ---- 写空闲位图 ---- */
	for (uint32_t i = 0; i < FSSIZE; i++) {
		if (!block_used[i])
			continue;
		image[BITMAP_BLOCK * BSIZE + i / 8] |= (1 << (i % 8));
	}

	/* ---- 写超级块 ---- */
	put32(image + 0, FSMAGIC);
	put32(image + 4, FSSIZE);
	put32(image + 8, NINODES);
	/* inodestart: inode 区从第几块开始。
	 * 布局: 块 0 = 超级块, 块 1 = 位图, 块 2.. = inode 区。
	 * 内核用同样的公式计算 (见 kernel/fs/bitmap.c 的 data_start_block),
	 * 两边必须一致 —— 否则内核会去错误的位置读 inode。 */
	put32(image + 12, INODE_START_BLOCK);
	put32(image + 16, 0);            /* 日志区大小 (本课程无日志) */
	put32(image + 20, (uint32_t)(argc - 2));  /* 根目录下的文件数 */

	/* ---- 输出镜像 ---- */
	FILE *out = fopen(argv[1], "wb");
	if (!out) {
		fprintf(stderr, "mkfs: 无法创建 %s\n", argv[1]);
		return 1;
	}
	fwrite(image, 1, sizeof(image), out);
	fclose(out);

	printf("mkfs: 已生成 %s (%d 块, %d KB)\n", argv[1], FSSIZE, FSSIZE * BSIZE / 1024);
	return 0;
}
