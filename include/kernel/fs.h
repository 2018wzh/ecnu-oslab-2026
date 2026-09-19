/* 文件系统接口 (generic kernel): 分层为 fd 表 -> file -> inode -> buf 缓存 -> 块设备抽象,
 * 每层只依赖下面一层, 文件系统不知道底下是 VirtIO 还是 SD 卡。 */
#ifndef __KERNEL_FS_H__
#define __KERNEL_FS_H__

#include <kernel/types.h>
#include <kernel/sync.h>
#define BSIZE      512      /* 块大小 (字节) */
#define FSMAGIC    0x10203040
#define NDIRECT    12       /* 直接块指针数量 */
#define NINDIRECT  (BSIZE / sizeof(uint32))  /* 一级间接块能存的指针数 */
#define MAXFILE    (NDIRECT + NINDIRECT + 1)
#define MAXNAME    14       /* 文件名最大长度 */
#define NINODES    200
#define ROOTINO    1        /* 根目录的 inode 号 */

/* inode 类型 */
#define T_DIR  1
#define T_FILE 2
#define T_DEV  3
typedef struct superblock {
	uint32 magic;
	uint32 size;        /* 总块数 */
	uint32 ninodes;
	uint32 inodestart;  /* inode 区起始块号 */
	uint32 nfiles;      /* 根目录下的文件数 */
} superblock_t;

/* --------------------------------------------------------------------------
 * 磁盘上的 inode
 * -------------------------------------------------------------------------- */
typedef struct dinode {
	uint16 type;                  /* T_DIR / T_FILE / T_DEV, 0 表示空闲 */
	uint16 major;                 /* 设备号 (T_DEV 时使用) */
	uint16 minor;
	uint16 nlink;                 /* 硬链接数 */
	uint32 size;                  /* 文件大小 (字节) */
	uint32 addrs[NDIRECT + 1];    /* 数据块号; 最后一个是一级间接块 */
} __attribute__((packed)) dinode_t;
STATIC_ASSERT(sizeof(dinode_t) == 64,
              "dinode_t 必须正好 64 字节, 与 tools/mkfs/mkfs.c 的磁盘格式一致");

/* --------------------------------------------------------------------------
 * 目录项
 * -------------------------------------------------------------------------- */
typedef struct dirent {
	uint16 inum;              /* inode 号, 0 表示此项空闲 */
	char name[MAXNAME];       /* 文件名 (定长, 不保证 '\0' 结尾!) */
} dirent_t;
typedef struct inode {
	uint32 inum;              /* inode 号 (0 表示未使用) */
	int ref;                  /* 引用计数: 有多少地方在用 */
	int valid;                /* 是否已从磁盘读入 */
	uint16 type;
	uint16 major, minor;
	uint16 nlink;
	uint32 size;
	uint32 addrs[NDIRECT + 1];
	sleeplock_t lock;         /* 保护读写: 磁盘 I/O 耗时, 用睡眠锁 */
} inode_t;





// 文件对象: 一次"打开"操作, 记录读写偏移与访问模式; 一个 inode 可有多个 file。
void fs_init(void);

/* 文件系统是否已成功挂载 */
int fs_mounted(void);

/* 列出根目录内容 (调试用) */
void fs_list_root(void);

/* 按 inode 号取得 inode (增加引用计数) */
inode_t *inode_get(uint32 inum);

/* 减少引用计数; 减到 0 且 nlink 也为 0 时释放 */
void inode_put(inode_t *ip);

/* 按路径查找 inode */
inode_t *inode_by_path(const char *path);

/* 按路径查找或创建 */
inode_t *inode_by_path_parent(const char *path, char *name);

/* 在目录中创建文件 */
inode_t *inode_create(inode_t *dir, const char *name, uint16 type,
                      uint16 major, uint16 minor);

/* 读取/写入 inode 数据 */
int inode_read(inode_t *ip, uint64 user_dst, uint32 off, uint32 n, int is_user);
int inode_write(inode_t *ip, uint64 user_src, uint32 off, uint32 n, int is_user);

/* 目录操作 */
int dir_link(inode_t *dir, const char *name, uint32 inum);
int dir_lookup(inode_t *dir, const char *name, uint32 *out_inum);

/* 打印 inode 信息 (调试) */
void inode_stat(inode_t *ip);

/* 初始化 inode 缓存 */
void inode_init(void);

/* 把内存 inode 写回磁盘 (调用者必须持有 ip->lock) */
void inode_update(inode_t *ip);



// 文件对象与描述符表 (kernel/fs/file.c)。
typedef struct buf {
	uint32 blockno;
	uint32 refcnt;            /* 引用计数 */
	int valid;                /* 数据是否有效 */
	sleeplock_t lock;
	struct buf *prev;
	struct buf *next;
	uint8 data[BSIZE];
} buf_t;

void bio_init(void);
buf_t *bread(uint32 blockno);
void bwrite(buf_t *b);
void brelse(buf_t *b);

/* 块分配 (kernel/fs/bitmap.c) */
uint32 balloc(void);
void bfree(uint32 blockno);

/* 读取并校验超级块; 返回 0 成功 */
int fs_read_superblock(void);
superblock_t *fs_superblock(void);

#endif /* __KERNEL_FS_H__ */
