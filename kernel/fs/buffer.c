#include <kernel/fs.h>
#include <kernel/print.h>
buffer_t buffer_cache[N_BUFFER], active_head, inactive_head;
spinlock_t buffer_lock;
/* 教师链表辅助：只维护链接，不改变引用数、内容或回收策略。 */
void buffer_move(buffer_t *node, bool active, bool front)
{
    assert(spinlock_holding(&buffer_lock), "buffer_move: cache lock");
    assert(node && node != &active_head && node != &inactive_head, "buffer_move: node");
    assert((node->next == NULL) == (node->prev == NULL), "buffer_move: links");
    if (node->next) {
        node->next->prev = node->prev;
        node->prev->next = node->next;
    }
    buffer_t *head = active ? &active_head : &inactive_head;
    buffer_t *left = front ? head : head->prev;
    buffer_t *right = front ? head->next : head;
    node->prev = left;
    node->next = right;
    left->next = node;
    right->prev = node;
}

/* data 指针的发布/清空也需 buffer_lock；页内容和 valid 仍由睡眠锁保护。
 * 只在 N_BUFFER_TEST 下输出完整快照，不在内核栈上分配 16384 行。 */
void buffer_print_info(void)
{
    if (N_BUFFER != N_BUFFER_TEST) { printf("buffer detail requires N_BUFFER_TEST=8\n"); return; }
    struct { uint32 block, refs; int index; bool active; void *data; } rows[N_BUFFER_TEST];
    size_t count = 0;
    spinlock_acquire(&buffer_lock);
    for (int list = 0; list < 2; ++list) {
        buffer_t *head = list == 0 ? &active_head : &inactive_head;
        for (buffer_t *node = head->next; node != head; node = node->next) {
            int index = 0;
            while (index < N_BUFFER && node != &buffer_cache[index]) ++index;
            assert(index < N_BUFFER && count < N_BUFFER_TEST, "buffer_print_info: list");
            rows[count].block = node->block;
            rows[count].refs = node->refs;
            rows[count].index = index;
            rows[count].data = node->data;
            rows[count++].active = list == 0;
        }
    }
    spinlock_release(&buffer_lock);
    printf("buffer cache (head to tail):\n");
    for (size_t i = 0; i < count; ++i)
        printf("%s buffer %d(ref = %d): page(pa = %p) -> block[%d]\n",
               rows[i].active ? "active" : "inactive", rows[i].index,
               rows[i].refs, rows[i].data, rows[i].block);
}
// TODO(lab-7): 初始化两条带哨兵双向循环链表、refs、睡眠锁；data 按需分配。
void buffer_init(void) { panic("TODO(lab-7): buffer_init"); }
// TODO(lab-7): 全局锁内查找/引用/选择 LRU；解全局锁后取得睡眠锁，miss 读盘。
buffer_t *buffer_get(uint32 block) { (void)block; panic("TODO(lab-7): buffer_get"); }
// TODO(lab-7): 释放睡眠锁后减少 refs，降至 0 移入 inactive 头部。
void buffer_put(buffer_t *b) { (void)b; panic("TODO(lab-7): buffer_put"); }
// TODO(lab-7): 要求持有 buffer 睡眠锁，同步写盘，失败沿用 panic 契约。
void buffer_write(buffer_t *b) { (void)b; panic("TODO(lab-7): buffer_write"); }
// TODO(lab-7): 只释放 inactive 尾部无人引用且无 I/O 的页，并清 valid。
uint32 buffer_freemem(uint32 count) { (void)count; panic("TODO(lab-7): buffer_freemem"); }

// TODO(lab-7): 检查持有睡眠锁后同步读盘，与 buffer_write 独立。
void buffer_read(buffer_t *b) { (void)b; panic("TODO(lab-7): buffer_read"); }
