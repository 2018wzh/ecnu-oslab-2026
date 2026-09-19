/* 块设备抽象接口: fs/ 只知道这个抽象, 具体驱动实现两点 (virtio_blk / sdhci)。
 * 读写以块 (512 字节) 为单位且同步; buf 须为内核可直接访问的物理地址。 */
#ifndef __KERNEL_BLOCK_H__
#define __KERNEL_BLOCK_H__

#include <kernel/types.h>

#define BLOCK_SIZE 512

// 块设备操作集合, 用函数指针 (ops): 块设备运行期可多实例, 用虚表按实例分派。
struct block_device;

struct block_ops {
	// 从第 blockno 块起读 nblocks 块到 buf, 成功返回 0, 失败返回负数。
	int (*read)(struct block_device *dev, uint64 blockno,
	            void *buf, uint32 nblocks);
	// 把 buf 的 nblocks 块写入第 blockno 块起的位置。
	int (*write)(struct block_device *dev, uint64 blockno,
	             const void *buf, uint32 nblocks);
};

struct block_device {
	const char *name;              /* 设备名, 用于调试打印 */
	struct block_ops *ops;         /* 操作集合 */
	void *priv;                    /* 设备私有数据 (驱动自己解释) */
	uint64 nblocks;                /* 设备总块数 (容量) */
};

// 注册一个块设备, 使其成为系统默认磁盘; 由平台驱动初始化时调用。
void block_register(struct block_device *dev);

// 取得默认块设备 (文件系统用)。
struct block_device *block_default(void);

// 供文件系统调用的统一入口, fs/ 只依赖这两个函数, 不关心底层设备。
int block_read(uint64 blockno, void *buf, uint32 nblocks);
int block_write(uint64 blockno, const void *buf, uint32 nblocks);

// 设备初始化入口 (各驱动实现, 名字因平台而异)。
void block_init(void);

#endif /* __KERNEL_BLOCK_H__ */
