#include <kernel/proc.h>
#include <kernel/mem.h>
#include <kernel/print.h>
#include <asm/csr.h>
#include <platform.h>
pgtbl_t kernel_pgtbl;
/* TODO(lab-2): 遍历三级页表，返回叶项地址；va < VA_MAX。
 * 不创建且路径缺失时返回 NULL；alloc 为真时从内核池分配缺失中间表，耗尽 panic。
 * 本章只支持 4KiB 叶项。调用者保证页表有效、独占修改，无并发遍历。
 */
pte_t *vm_getpte(pgtbl_t root, uint64 va, bool alloc)
{ (void)root; (void)va; (void)alloc; panic("TODO(lab-2): vm_getpte"); }
/* TODO(lab-2): 映射 [va,va+len) 到 [pa,pa+len)，错误 panic。
 * va/pa 页对齐，len > 0，末页向上覆盖；加法及取整不得溢出，
 * 覆盖范围不得超过 VA_MAX 或 56 位物理地址范围。
 * perm 仅含 R/W/X/U/G/A/D，R 或 X 至少一个，W 必须同时有 R。
 * 叶项设置 V/A/D；同一 VA/PA 可更新权限，改指向另一 PA 则 panic。
 */
void vm_mappages(pgtbl_t root, uint64 va, uint64 pa, uint64 len, uint64 perm)
{ (void)root; (void)va; (void)pa; (void)len; (void)perm; panic("TODO(lab-2): vm_mappages"); }
/* TODO(lab-2): va 页对齐、len > 0，末页向上覆盖，范围及溢出约束同映射。
 * 解除缺失映射则 panic；清除叶项，可选归还普通池数据页，不回收中间页表。
 * free_pages 为真时，调用者须已消除其他使用者/别名，不能用于设备或内核页。
 */
void vm_unmappages(pgtbl_t root, uint64 va, uint64 len, bool free_pages)
{ (void)root; (void)va; (void)len; (void)free_pages; panic("TODO(lab-2): vm_unmappages"); }
// TODO(lab-2): 映射内核代码 RX、只读区 R、数据与可分配区 RW、UART 和 PLIC RW。
// 不设置 U，不映射固件保留区；CLINT 由固件管理，PLIC 映射沿用 lab-2，中断驱动由 trap 模块使用。
// TODO(lab-4): 同时映射 trampoline RX（不设 U），内核池分配首进程栈页并映射 KSTACK(0) RW。
// 保留相邻保护页及 TRAPFRAME 对应位置不映射；在激活/发布页表前完成。
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
        for (unsigned n = level; n < 3; ++n) printf(".. ");
        printf("level=%d index=%d pa=%p flags=%x\n", (int)level, (int)i,
               (void *)PTE_TO_PA(pte), (unsigned)(pte & 1023));
        if (level && !(pte & (PTE_R | PTE_W | PTE_X))) print_level((pgtbl_t)PTE_TO_PA(pte), level - 1);
    }
}
/* 教师诊断：root 及所有中间表须有效，遍历期间不允许修改。 */
void vm_print(pgtbl_t root)
{ printf("root pgtbl: pa=%p\n", (void *)root); print_level(root, 2); }
