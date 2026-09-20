#ifndef OSLAB_VIRTIO_BLK_H
#define OSLAB_VIRTIO_BLK_H
#include <kernel/types.h>
typedef struct { uint64 address; uint32 len; uint16 flags, next; } virtq_desc_t;
typedef struct { uint32 id, len; } virtq_used_t;
typedef struct __attribute__((aligned(4096))) {
    virtq_desc_t desc[8];
    struct { uint16 flags, index, ring[8], used_event; } avail;
    struct { uint16 flags, index; virtq_used_t ring[8]; uint16 avail_event; } used;
    uint8 status[8];
} virtio_queue_t;
typedef struct { uint64 base; virtio_queue_t *queue; uint16 consumed; bool free[8]; uint64 queue_pa; void (*fence)(void); } virtio_blk_t;
// Queue 与 data 必须是稳定、DMA 可达的物理连续内存；调用者串行化访问。
int virtio_blk_init(virtio_blk_t *disk, uint64 base, virtio_queue_t *queue, uint64 queue_pa, void (*fence)(void));
typedef struct __attribute__((aligned(16))) { uint32 type, reserved; uint64 sector; } virtio_header_t;
int virtio_blk_submit(virtio_blk_t *disk, uint64 header_pa, uint64 data_pa, bool write);
void virtio_blk_release(virtio_blk_t *disk, int head);
int virtio_blk_complete(virtio_blk_t *disk, int *result); // 返回头描述符，-1 表示无完成
#endif
