#include <kernel/mem.h>
#include <kernel/print.h>
#include <asm/csr.h>
#include <platform.h>
pgtbl_t kernel_pgtbl;
// TODO(lab-2): 遍历三级页表；alloc 为真时分配缺失的中间页表。
pte_t *walk(pgtbl_t root, uint64 va, bool alloc)
{ (void)root; (void)va; (void)alloc; panic("TODO(lab-2): walk"); }
// TODO(lab-2): 建立对齐范围的 4KiB 映射；拒绝覆盖，失败返回 -1。
int map(pgtbl_t root, uint64 va, uint64 pa, uint64 len, uint64 perm)
{ (void)root; (void)va; (void)pa; (void)len; (void)perm; panic("TODO(lab-2): map"); }
// TODO(lab-2): 清除叶 PTE；free_pages 为真时归还普通池页面。
void unmap(pgtbl_t root, uint64 va, uint64 len, bool free_pages)
{ (void)root; (void)va; (void)len; (void)free_pages; panic("TODO(lab-2): unmap"); }
// TODO(lab-2): 映射内核代码 RX、只读区 R、数据与可分配区 RW、UART RW。
void kvm_init(void) { panic("TODO(lab-2): kvm_init"); }
void kvm_inithart(void)
{
    csr_write(satp, (8UL << 60) | ((uint64)kernel_pgtbl >> 12));
    __asm__ volatile("sfence.vma zero, zero" ::: "memory");
}
static void print_level(pgtbl_t root, unsigned level)
{
    for (unsigned i = 0; i < 512; i++) {
        pte_t pte = root[i];
        if (!(pte & PTE_V)) continue;
        printf("level=%d index=%d pa=%p flags=%x\n", (int)level, (int)i,
               (void *)PTE_TO_PA(pte), (unsigned)(pte & 1023));
        if (level && !(pte & (PTE_R | PTE_W | PTE_X))) print_level((pgtbl_t)PTE_TO_PA(pte), level - 1);
    }
}
void vm_print(pgtbl_t root) { print_level(root, 2); }
