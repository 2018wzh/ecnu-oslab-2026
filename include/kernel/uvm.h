#ifndef OSLAB_UVM_H
#define OSLAB_UVM_H
#include <kernel/proc.h>
#define MMAP_END (USER_STACK_TOP - 4096 * PAGE_SIZE)
#define MMAP_BEGIN (MMAP_END - 16384 * PAGE_SIZE)
#define N_MMAP 256
/* 描述已分配区域，按 begin 升序链接；匿名页固定 RWU。 */
typedef struct mmap_region { uint64 begin, pages; struct mmap_region *next; } mmap_region_t;
typedef struct mmap_region_node { mmap_region_t mmap; struct mmap_region_node *next; } mmap_region_node_t;
void mmap_init(void);
void mmap_show_nodelist(void);
mmap_region_t *mmap_region_alloc(void);
void mmap_region_free(mmap_region_t *node);
/* 调用者独占链表；合并释放一个节点，不修改 next，调用者须修复链接。
 * 非法/不相邻返回 NULL 且不修改；预期合并失败时调用者 panic。 */
mmap_region_t *mmap_merge_regions(mmap_region_t *left, mmap_region_t *right, bool keep_left);
void uvm_show_mmaplist(const mmap_region_t *head);
/* 连续已映射用户页；目标独占且未发布，无特殊页；非法输入或映射失败 panic。 */
void uvm_copy_range(pgtbl_t source, pgtbl_t target, uint64 begin, uint64 end);
void copy_from_user(proc_t *p, void *dst, uint64 src, size_t len);
void copy_to_user(proc_t *p, uint64 dst, const void *src, size_t len);
/* 最多复制 maxlen 字节，遇 NUL 提前结束；达到上限无需强行补 NUL。 */
void copy_str_from_user(proc_t *p, char *dst, uint64 src, size_t maxlen);
uint64 uvm_heap_grow(pgtbl_t root, uint64 top, uint64 len);
uint64 uvm_heap_ungrow(pgtbl_t root, uint64 top, uint64 len);
/* 成功更新 ustack_npage；非法栈地址 panic。 */
void uvm_stack_grow(proc_t *p, uint64 fault);
/* 字节长度；首次适配，找不到返回 0。 */
uint64 uvm_mmap_find(const mmap_region_t *head, uint64 len);
long uvm_mmap(proc_t *p, uint64 begin, uint64 len);
void uvm_munmap(proc_t *p, uint64 begin, uint64 len);
/* 不复制 frame/trampoline 或 mmap 节点；目标根独立、尚未发布。 */
void uvm_copy_pgtbl(pgtbl_t source, pgtbl_t target, uint64 heap_top, uint64 ustack_npage, const mmap_region_t *mmap);
/* 只能销毁已停止使用且独占的页表；释放 frame 后调用者必须清除其悬空指针，不能再次释放。 */
void uvm_destroy(pgtbl_t root);
#endif
