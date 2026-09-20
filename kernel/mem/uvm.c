#include <kernel/uvm.h>
#include <kernel/print.h>
#include <kernel/string.h>
/* 教师连续页复制；不选择区域，不复制特殊页，也不复制 mmap 节点。 */
void uvm_copy_range(pgtbl_t source, pgtbl_t target, uint64 begin, uint64 end)
{
    assert(source && target && source != target && begin <= end && end <= TRAPFRAME &&
           begin % PAGE_SIZE == 0 && end % PAGE_SIZE == 0, "copy range");
    for (uint64 va = begin; va < end; va += PAGE_SIZE) {
        pte_t *pte = vm_getpte(source, va, false);
        assert(pte && (*pte & PTE_V) && (*pte & PTE_U) && (*pte & (PTE_R | PTE_W | PTE_X)), "copy source");
        void *page = pmem_alloc(false); /* 耗尽 panic；新普通页由目标独占。 */
        memcpy(page, (const void *)PTE_TO_PA(*pte), PAGE_SIZE);
        vm_mappages(target, va, (uint64)page, PAGE_SIZE, *pte & (PTE_R | PTE_W | PTE_X | PTE_U | PTE_G | PTE_A | PTE_D));
    }
}
/* 只访问调用者独占的已分配区域链。 */
void uvm_show_mmaplist(const mmap_region_t *head)
{
    printf("\nalloced mmap_space:\n");
    if (!head) printf("empty\n");
    unsigned count = 0;
    for (; head && count < N_MMAP; head = head->next, ++count)
        printf("mmap begin=%p pages=%d\n", (void *)head->begin, (int)head->pages);
    assert(!head, "mmap list cycle or too many nodes");
}
// TODO(lab-5): 手动查询用户页表，支持源/目标非对齐及跨页；非法地址可断言或 panic。
void copy_from_user(proc_t *p, void *dst, uint64 src, size_t len)
{ (void)p; (void)dst; (void)src; (void)len; panic("TODO(lab-5): copy_from_user"); }
// TODO(lab-5): 同上，逐页向可写用户内存复制；不要直接解引用用户 VA。
void copy_to_user(proc_t *p, uint64 dst, const void *src, size_t len)
{ (void)p; (void)dst; (void)src; (void)len; panic("TODO(lab-5): copy_to_user"); }
// TODO(lab-5): 最多复制 maxlen 字节，遇 NUL 终止；支持非对齐及跨页，不新增超长报错契约。
void copy_str_from_user(proc_t *p, char *dst, uint64 src, size_t maxlen)
{ (void)p; (void)dst; (void)src; (void)maxlen; panic("TODO(lab-5): copy_str_from_user"); }
// TODO(lab-5): 独立堆增长：按页申请并清零普通池页面，映射 flags|U，返回新堆顶，不超过 MMAP_BEGIN。
uint64 uvm_heap_grow(pgtbl_t root, uint64 top, uint64 len, uint64 flags)
{ (void)root; (void)top; (void)len; (void)flags; panic("TODO(lab-5): uvm_heap_grow"); }
// TODO(lab-5): 独立堆收缩：解除映射并归还普通页，返回新堆顶，不低于 0x2000。
uint64 uvm_heap_ungrow(pgtbl_t root, uint64 top, uint64 len)
{ (void)root; (void)top; (void)len; panic("TODO(lab-5): uvm_heap_ungrow"); }
// TODO(lab-5): 由 ustack_npage 推导栈底；合法缺页补齐到 fault 页，更新页数，非法地址 panic。
// 预留 4096 页，不越过 MMAP_END；只增长不收缩，调用者重试故障 PC。
void uvm_stack_grow(proc_t *p, uint64 fault)
{ (void)p; (void)fault; panic("TODO(lab-5): uvm_stack_grow"); }
// TODO(lab-5): 扫描有序的已分配链，在 [MMAP_BEGIN, MMAP_END) 找首个足够大空隙；失败返回 0。
uint64 uvm_mmap_find(const mmap_region_t *head, uint64 len)
{ (void)head; (void)len; panic("TODO(lab-5): uvm_mmap_find"); }
// TODO(lab-5): begin=0 调用 uvm_mmap_find；维护有序链，合并相邻区域，申请普通页并映射 RWU。
// 字节长度与地址由 syscall 校验；重叠、找不到空间或底层失败 panic，不要求回滚。
long uvm_mmap(proc_t *p, uint64 begin, uint64 len)
{ (void)p; (void)begin; (void)len; panic("TODO(lab-5): uvm_mmap"); }
// TODO(lab-5): 头尾裁剪、中间拆分、跨节点解除，回收普通页与空节点；底层失败 panic。
void uvm_munmap(proc_t *p, uint64 begin, uint64 len)
{ (void)p; (void)begin; (void)len; panic("TODO(lab-5): uvm_munmap"); }
// TODO(lab-5): 按代码、堆、栈、mmap 区域调用连续页辅助；保留空洞和权限，深复制普通页。
// source 稳定存活，target 根独立且未发布；不复制 frame/trampoline、进程字段或节点（lab-6）。
void uvm_copy_pgtbl(pgtbl_t source, pgtbl_t target, uint64 heap_top, uint64 ustack_npage, const mmap_region_t *mmap)
{ (void)source; (void)target; (void)heap_top; (void)ustack_npage; (void)mmap; panic("TODO(lab-5): uvm_copy_pgtbl"); }
// TODO(lab-5): 递归回收普通池用户叶页和内核池各级页表页；顶级 level=3，跳过无效项。
static void destroy_pgtbl(pgtbl_t root, unsigned level)
{ (void)root; (void)level; panic("TODO(lab-5): destroy_pgtbl"); }
/* 教师销毁入口：frame 独占且来自内核池；trampoline 共享，仅解除映射。
 * root 已停止使用；frame 指针在销毁后失效，调用方不得二次释放。
 */
void uvm_destroy(pgtbl_t root)
{
    if (!root) return;
    const uint64 special[] = {TRAPFRAME, TRAMPOLINE};
    for (unsigned i = 0; i < 2; ++i) {
        pte_t *pte = vm_getpte(root, special[i], false);
        if (pte && (*pte & PTE_V)) {
            uint64 pa = PTE_TO_PA(*pte);
            vm_unmappages(root, special[i], PAGE_SIZE, false);
            if (special[i] == TRAPFRAME) pmem_free(pa, true);
        }
    }
    destroy_pgtbl(root, 3);
}

// TODO(lab-9): heap_grow 支持输入 R/W/X 权限并加 U；exec 的字节堆顶按页覆盖映射且清零新页。
// 普通 brk 仍沿用前序页对齐契约并传 RW。
