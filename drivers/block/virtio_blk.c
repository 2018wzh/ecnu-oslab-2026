/* VirtIO 块设备驱动 (QEMU 平台), 通过共享内存 virtqueue 通信, 轮询等待完成。 */

#include <kernel/types.h>
#include <kernel/print.h>
#include <kernel/block.h>
#include <kernel/string.h>
#include <kernel/mm.h>
#include <kernel/arch.h>
#include <platform.h>

/* ---- VirtIO MMIO 寄存器偏移 (virtio 1.x 规范) ---- */
#define VIRTIO_MMIO_MAGIC           0x000
#define VIRTIO_MMIO_VERSION         0x004
#define VIRTIO_MMIO_DEVICE_ID       0x008
#define VIRTIO_MMIO_VENDOR_ID       0x00c
#define VIRTIO_MMIO_DEVICE_FEATURES 0x010
#define VIRTIO_MMIO_DRIVER_FEATURES 0x020
#define VIRTIO_MMIO_QUEUE_SEL       0x030
#define VIRTIO_MMIO_QUEUE_NUM_MAX   0x034
#define VIRTIO_MMIO_QUEUE_NUM       0x038
#define VIRTIO_MMIO_QUEUE_READY     0x044
#define VIRTIO_MMIO_QUEUE_NOTIFY    0x050
#define VIRTIO_MMIO_INTERRUPT_STATUS 0x060
#define VIRTIO_MMIO_INTERRUPT_ACK   0x064
#define VIRTIO_MMIO_STATUS          0x070
#define VIRTIO_MMIO_GUEST_PAGE_SIZE 0x028
#define VIRTIO_MMIO_QUEUE_PFN       0x040

#define VIRTIO_MMIO_QUEUE_DESC_LOW  0x080
#define VIRTIO_MMIO_QUEUE_DESC_HIGH 0x084
#define VIRTIO_MMIO_DRIVER_DESC_LOW 0x090
#define VIRTIO_MMIO_DRIVER_DESC_HIGH 0x094
#define VIRTIO_MMIO_DEVICE_DESC_LOW 0x0a0
#define VIRTIO_MMIO_DEVICE_DESC_HIGH 0x0a4

#define VIRTIO_MAGIC_VALUE  0x74726976   /* "virt" */
#define VIRTIO_DEVICE_ID_BLK 2

/* 状态位 */
#define VIRTIO_STATUS_ACKNOWLEDGE 1
#define VIRTIO_STATUS_DRIVER      2
#define VIRTIO_STATUS_DRIVER_OK   4
#define VIRTIO_STATUS_FEATURES_OK 8

/* 描述符标志 */
#define VRING_DESC_F_NEXT  1
#define VRING_DESC_F_WRITE 2

/* 块设备请求类型 */
#define VIRTIO_BLK_T_IN  0
#define VIRTIO_BLK_T_OUT 1

#define VIRTIO_QUEUE_SIZE 8

/* ---- virtqueue 结构 ---- */
typedef struct vring_desc {
	uint64 addr;
	uint32 len;
	uint16 flags;
	uint16 next;
} vring_desc_t;

typedef struct vring_avail {
	uint16 flags;
	uint16 idx;
	uint16 ring[VIRTIO_QUEUE_SIZE];
} vring_avail_t;

typedef struct vring_used_elem {
	uint32 id;
	uint32 len;
} vring_used_elem_t;

typedef struct vring_used {
	uint16 flags;
	uint16 idx;
	vring_used_elem_t ring[VIRTIO_QUEUE_SIZE];
} vring_used_t;

/* 块设备请求头 (与 VirtIO 规范一致, 不能随意改动布局) */
typedef struct virtio_blk_req {
	uint32 type;
	uint32 reserved;
	uint64 sector;   /* 起始扇区号 (512 字节为单位) */
} virtio_blk_req_t;

#define REG32(off) (*(volatile uint32 *)(PLAT_VIRTIO0_BASE + (off)))
typedef struct virtqueue_mem {
	/* 第 0 页: 描述符表 + avail ring */
	vring_desc_t desc[VIRTIO_QUEUE_SIZE];   /* 8 * 16 = 128 字节 */
	vring_avail_t avail;                    /* 紧随其后 */
	uint8 _pad[4096 - (sizeof(vring_desc_t) * VIRTIO_QUEUE_SIZE
	                   + sizeof(vring_avail_t))];

	/* 第 1 页: used ring (必须页对齐) */
	vring_used_t used;
} __attribute__((aligned(4096))) virtqueue_mem_t;

static struct {
	virtqueue_mem_t guest_pages __attribute__((aligned(4096)));

	/* 每个请求需要的数据缓冲区, 对齐便于观察也对 DMA 友好 */
	uint8 data[VIRTIO_QUEUE_SIZE][BLOCK_SIZE] __attribute__((aligned(4096)));

	virtio_blk_req_t req[VIRTIO_QUEUE_SIZE];
	uint8 status[VIRTIO_QUEUE_SIZE];

	int ready;
	int version;
	uint16 used_idx;
} vd;

/* 超时保护: 如果设备永远不响应, 我们不能无限等待 */
#define VIRTIO_TIMEOUT_CYCLES (100000000UL)

static int virtio_init_device(void)
{
	uint32 magic = REG32(VIRTIO_MMIO_MAGIC);
	if (magic != VIRTIO_MAGIC_VALUE) {
		printf("[virtio] 魔数错误: 0x%x (期望 0x%x)\n",
		       magic, VIRTIO_MAGIC_VALUE);
		return -1;
	}

	vd.version = (int)REG32(VIRTIO_MMIO_VERSION);
	printf("[virtio] MMIO 版本 = %d (%s)\n", vd.version,
	       vd.version == 1 ? "旧版 legacy" : "新版 modern");
	uint32 dev_id = REG32(VIRTIO_MMIO_DEVICE_ID);
	if (dev_id != VIRTIO_DEVICE_ID_BLK) {
		printf("[virtio] 设备类型不是块设备 (device_id=%u)\n", dev_id);
		return -1;
	}
	REG32(VIRTIO_MMIO_STATUS) = 0;                      /* 复位 */
	REG32(VIRTIO_MMIO_STATUS) |= VIRTIO_STATUS_ACKNOWLEDGE;
	REG32(VIRTIO_MMIO_STATUS) |= VIRTIO_STATUS_DRIVER;

	/* 读取设备特性 (本驱动不使用任何可选特性, 但必须读一次,
	 * 否则某些实现会认为驱动不合规) */
	(void)REG32(VIRTIO_MMIO_DEVICE_FEATURES);
	REG32(VIRTIO_MMIO_DRIVER_FEATURES) = 0;

	/* 选择队列 0 并设置大小 */
	REG32(VIRTIO_MMIO_QUEUE_SEL) = 0;
	uint32 qmax = REG32(VIRTIO_MMIO_QUEUE_NUM_MAX);
	if (qmax == 0) {
		printf("[virtio] 队列不可用\n");
		return -1;
	}
	if (qmax < VIRTIO_QUEUE_SIZE) {
		printf("[virtio] 队列太小: %u < %d\n", qmax, VIRTIO_QUEUE_SIZE);
		return -1;
	}
	REG32(VIRTIO_MMIO_QUEUE_NUM) = VIRTIO_QUEUE_SIZE;
	memset(&vd.guest_pages, 0, sizeof(vd.guest_pages));

	if (vd.version == 1) {
		REG32(VIRTIO_MMIO_GUEST_PAGE_SIZE) = 4096;

		uint64 base = (uint64)&vd.guest_pages;
		uint32 pfn = (uint32)(base >> 12);

		REG32(VIRTIO_MMIO_QUEUE_PFN) = pfn;
	} else {
		/* ---- 新版 (modern) 路径: 分别告知三个区域的物理地址 ---- */
		uint64 d = (uint64)&vd.guest_pages.desc;
		uint64 a = (uint64)&vd.guest_pages.avail;
		uint64 u = (uint64)&vd.guest_pages.used;

		REG32(VIRTIO_MMIO_QUEUE_DESC_LOW)   = (uint32)(d & 0xffffffff);
		REG32(VIRTIO_MMIO_QUEUE_DESC_HIGH)  = (uint32)(d >> 32);
		REG32(VIRTIO_MMIO_DRIVER_DESC_LOW)  = (uint32)(a & 0xffffffff);
		REG32(VIRTIO_MMIO_DRIVER_DESC_HIGH) = (uint32)(a >> 32);
		REG32(VIRTIO_MMIO_DEVICE_DESC_LOW)  = (uint32)(u & 0xffffffff);
		REG32(VIRTIO_MMIO_DEVICE_DESC_HIGH) = (uint32)(u >> 32);

		REG32(VIRTIO_MMIO_QUEUE_READY) = 1;
	}

	vd.ready = 1;
	vd.used_idx = 0;
	return 0;
}
static int virtio_rw(uint64 sector, void *buf, int is_write)
{
}

/* ---- block_ops 实现 ---- */
static int virtio_blk_read(struct block_device *dev, uint64 blockno,
                           void *buf, uint32 nblocks)
{
	(void)dev;
	if (!vd.ready)
		return -1;

	/* 统计总容量 (首次访问时通过读最后一个扇区探测 —— 简化处理:
	 * 直接用 mkfs 生成的 FSSIZE, 由超级块告诉文件系统真实大小) */
	for (uint32 i = 0; i < nblocks; i++) {
		if (virtio_rw((blockno + i) * (BLOCK_SIZE / 512),
		              (uint8 *)buf + i * BLOCK_SIZE, 0) < 0)
			return -1;
	}
	return 0;
}

static int virtio_blk_write(struct block_device *dev, uint64 blockno,
                            const void *buf, uint32 nblocks)
{
	(void)dev;
	if (!vd.ready)
		return -1;

	for (uint32 i = 0; i < nblocks; i++) {
		if (virtio_rw((blockno + i) * (BLOCK_SIZE / 512),
		              (uint8 *)buf + i * BLOCK_SIZE, 1) < 0)
			return -1;
	}
	return 0;
}

static struct block_ops virtio_blk_ops = {
	.read  = virtio_blk_read,
	.write = virtio_blk_write,
};

static struct block_device virtio_blk_dev = {
	.name    = "virtio-blk",
	.ops     = &virtio_blk_ops,
	.priv    = NULL,
	.nblocks = 0,
};

void block_init(void)
{
	printf("[block] 初始化 VirtIO 块设备 @ 0x%lx\n", (uint64)PLAT_VIRTIO0_BASE);

	if (virtio_init_device() < 0) {
		printf("[block] VirtIO 初始化失败\n");
		return;
	}

	/* 容量暂时填一个足够大的值; 真实大小由文件系统从超级块读取。
	 * 这是刻意的简化: 教学内核里"磁盘多大"由镜像内容决定,
	 * 不需要驱动去探测。 */
	virtio_blk_dev.nblocks = 4096;
	block_register(&virtio_blk_dev);

	printf("[block] VirtIO 块设备就绪 (sector 粒度 512 字节)\n");
}

/* VirtIO 不使用中断 (轮询模式), 提供空实现以保持符号完整 */
void virtio_disk_intr(void)
{
}
