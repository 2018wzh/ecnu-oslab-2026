#include <drivers/virtio_blk.h>
static uint32 rd(virtio_blk_t *d, unsigned off) { return *(volatile uint32 *)(d->base + off); }
static void wr(virtio_blk_t *d, unsigned off, uint32 v) { *(volatile uint32 *)(d->base + off) = v; }
static void addr(virtio_blk_t *d, unsigned off, uint64 pa) { wr(d, off, pa); wr(d, off + 4, pa >> 32); }
int virtio_blk_init(virtio_blk_t *d, uint64 base, virtio_queue_t *q, uint64 pa, void (*fence)(void))
{
    *d = (virtio_blk_t){ .base = base, .queue = q, .fence = fence, .queue_pa = pa };
    if (!base || rd(d, 0) != 0x74726976 || rd(d, 4) != 2 || rd(d, 8) != 2) return -1;
    wr(d, 0x70, 0); fence(); wr(d, 0x70, 1); wr(d, 0x70, 3);
    wr(d, 0x14, 0); if (rd(d, 0x10) & (1U << 5)) return -1; // 不支持只读设备
    wr(d, 0x14, 1); if (!(rd(d, 0x10) & 1)) return -1; // VERSION_1
    wr(d, 0x24, 0); wr(d, 0x20, 0); wr(d, 0x24, 1); wr(d, 0x20, 1);
    wr(d, 0x70, 11); fence(); if (!(rd(d, 0x70) & 8)) return -1;
    wr(d, 0x30, 0); if (rd(d, 0x44) || rd(d, 0x34) < 8) return -1;
    for (size_t i = 0; i < sizeof(*q); ++i) ((uint8 *)q)[i] = 0;
    wr(d, 0x38, 8);
    addr(d, 0x80, pa + offsetof(virtio_queue_t, desc));
    addr(d, 0x90, pa + offsetof(virtio_queue_t, avail));
    addr(d, 0xa0, pa + offsetof(virtio_queue_t, used));
    for (int i = 0; i < 8; ++i) d->free[i] = true;
    fence(); wr(d, 0x44, 1); wr(d, 0x70, 15); return 0;
}
/* OS 条件锁保护分配、提交、完成与归还；资源不足不发布部分链。 */
int virtio_blk_submit(virtio_blk_t *d, uint64 header_pa, uint64 data_pa, bool write)
{
    int ids[3], n = 0;
    for (int i = 0; i < 8 && n < 3; ++i) if (d->free[i]) ids[n++] = i;
    if (n != 3) return -1;
    for (int i = 0; i < 3; ++i) d->free[ids[i]] = false;
    int h = ids[0];
    virtio_queue_t *q = d->queue;
    q->status[h] = 0xff;
    q->desc[h] = (virtq_desc_t){header_pa, 16, 1, ids[1]};
    q->desc[ids[1]] = (virtq_desc_t){data_pa, 4096, write ? 1 : 3, ids[2]};
    q->desc[ids[2]] = (virtq_desc_t){d->queue_pa + offsetof(virtio_queue_t, status) + h, 1, 2, 0};
    uint16 index = q->avail.index;
    q->avail.ring[index % 8] = h;
    d->fence(); *(volatile uint16 *)&q->avail.index = index + 1;
    d->fence(); wr(d, 0x50, 0);
    return h;
}
void virtio_blk_release(virtio_blk_t *d, int head)
{
    for (;;) {
        virtq_desc_t entry = d->queue->desc[head];
        d->queue->desc[head] = (virtq_desc_t){0};
        d->free[head] = true;
        if (!(entry.flags & 1)) break;
        head = entry.next;
    }
}
int virtio_blk_complete(virtio_blk_t *d, int *result)
{
    uint32 irq = rd(d, 0x60); if (irq) wr(d, 0x64, irq & 3);
    if (*(volatile uint16 *)&d->queue->used.index == d->consumed) return -1;
    d->fence(); uint32 id = *(volatile uint32 *)&d->queue->used.ring[d->consumed % 8].id;
    /* 非法设备响应不能归还仍可能由设备持有的内存。 */
    if (id >= 8 || d->free[id]) for (;;) {}
    *result = *(volatile uint8 *)&d->queue->status[id] == 0 ? 0 : -1;
    d->consumed++;
    return id;
}
