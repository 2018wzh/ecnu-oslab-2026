#include <kernel/fs.h>
#include <kernel/mem.h>
#include <kernel/print.h>
#include <kernel/proc.h>
#include <kernel/arch.h>
#include <stdatomic.h>
#include <drivers/virtio_blk.h>
#include <drivers/sd.h>
#include <platform.h>
#if !BLOCK_IS_SD
static virtio_blk_t device;
static virtio_queue_t queue;
#endif
static spinlock_t request_lock;
static buffer_t *active[8];
static atomic_bool ready;
/* 教师驱动外围。启动单次调用，学生负责 main、页表、PLIC 与 trap 接入。 */
int block_init(void)
{
    spinlock_init(&request_lock, "block request");
    int result;
#if BLOCK_IS_SD
    result = sd_init();
#else
    result = virtio_blk_init(&device, BLOCK_BASE, &queue, (uint64)&queue, arch_dma_fence);
#endif
    if (result) return -1;
    atomic_store_explicit(&ready, true, memory_order_release);
    return 0;
}
int block_rw(buffer_t *b, bool write)
{
    if (!atomic_load_explicit(&ready, memory_order_acquire) || !b || !b->data || b->block >= TOTAL_BLOCKS) return -1;
    uint64 pa = (uint64)b->data;
    if (pa % BLOCK_SIZE || pa < DRAM_BASE || pa > DRAM_BASE + DRAM_SIZE - BLOCK_SIZE) return -1;
    assert(sleeplock_holding(&b->lock), "block_rw: buffer lock");
    /* 栈头 16 字节对齐，不跨页；睡眠只切换栈，不释放或移动它。 */
    virtio_header_t header = {write ? 1 : 0, 0, (uint64)b->block * 8};
    uint64 header_pa = kvm_translate((uint64)&header);
    spinlock_acquire(&request_lock);
    int id;
#if BLOCK_IS_SD
    (void)header_pa;
    while (active[0]) proc_sleep(active, &request_lock);
    id = 0;
    if (sd_submit(pa, b->block, write)) { spinlock_release(&request_lock); return -1; }
#else
    while ((id = virtio_blk_submit(&device, header_pa, pa, write)) < 0)
        proc_sleep(active, &request_lock);
#endif
    active[id] = b; b->disk = true; b->io_result = -1;
    while (b->disk) proc_sleep(b, &request_lock);
    int result = b->io_result;
#if !BLOCK_IS_SD
    virtio_blk_release(&device, id);
#endif
    active[id] = NULL;
    proc_wakeup(active);
    spinlock_release(&request_lock);
    return result;
}
void block_interrupt(void)
{
    if (!atomic_load_explicit(&ready, memory_order_acquire)) return;
    spinlock_acquire(&request_lock);
    int id, result;
#if BLOCK_IS_SD
    id = sd_complete(&result) ? 0 : -1;
    if (id >= 0) {
#else
    while ((id = virtio_blk_complete(&device, &result)) >= 0) {
#endif
        assert(active[id] && active[id]->disk, "unexpected block completion");
        active[id]->io_result = result; active[id]->disk = false;
        proc_wakeup(active[id]);
    }
    spinlock_release(&request_lock);
}
/* TODO(lab-7): 映射块设备 MMIO；VF2 还需 CCACHE_BASE 的 16KiB RW 非用户映射。 */
void block_map(void) { panic("TODO(lab-7): block_map"); }
