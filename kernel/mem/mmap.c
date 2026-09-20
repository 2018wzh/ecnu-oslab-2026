#include <kernel/uvm.h>
#include <kernel/print.h>
// mmap_region_node_t 仓库(单向链表) + 链表头节点(不可分配) + 保护仓库的自旋锁
static mmap_region_node_t node_list[N_MMAP];
static mmap_region_node_t list_head;
static spinlock_t list_lk;

// 输出可用的 mmap_region_node_t 链，供调试；持锁遍历打印；锁顺序为节点池锁 -> 打印锁，禁止反向获取。
void mmap_show_nodelist(void)
{
    unsigned count = 0;
    spinlock_acquire(&list_lk);
    mmap_region_node_t *node = list_head.next;
    while (node && count < N_MMAP) {
        unsigned index = 0;
        while (index < N_MMAP && node != &node_list[index]) ++index;
        if (index == N_MMAP) break;
        printf("node %d index = %d\n", (int)count++, (int)index);
        node = node->next;
    }
    bool valid = node == NULL;
    spinlock_release(&list_lk);
    assert(valid, "invalid mmap free list");
}
/* 教师合并辅助：不处理 next，不分配/释放用户页面。 */
mmap_region_t *mmap_merge_regions(mmap_region_t *left, mmap_region_t *right, bool keep_left)
{
    if (!left || !right || left == right || !left->pages || !right->pages ||
        left->begin < MMAP_BEGIN || right->begin >= MMAP_END ||
        left->begin % PAGE_SIZE || right->begin % PAGE_SIZE ||
        left->begin >= right->begin || left->pages != (right->begin - left->begin) / PAGE_SIZE ||
        right->pages > (MMAP_END - right->begin) / PAGE_SIZE) return NULL;
    mmap_region_t *keep = keep_left ? left : right;
    mmap_region_t *discard = keep_left ? right : left;
    uint64 begin = left->begin, pages = left->pages + right->pages;
    keep->begin = begin;
    keep->pages = pages;
    mmap_region_free(discard);
    return keep;
}
// TODO(lab-5): 按数组索引升序链接空闲节点，初始化不可分配的头节点和保护锁。
void mmap_init(void) { panic("TODO(lab-5): mmap_init"); }
// TODO(lab-5): 持锁取出并清空节点，耗尽 panic。
mmap_region_t *mmap_region_alloc(void) { panic("TODO(lab-5): mmap_region_alloc"); }
// TODO(lab-5): 检查归属，解除所有外部引用后归还节点。
void mmap_region_free(mmap_region_t *node)
{ (void)node; panic("TODO(lab-5): mmap_region_free"); }
