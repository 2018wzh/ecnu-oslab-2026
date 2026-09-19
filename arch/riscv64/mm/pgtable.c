// Sv39 页表操作 (架构后端)。
// generic VM 只说"请把 va 映射到 pa", 三级页表细节全在本文件。
#include <kernel/types.h>
#include <kernel/mm.h>
#include <kernel/arch.h>
#include <kernel/print.h>
#include <asm/pgtable.h>

/* 取得一个页表项所在的页表页 */
static pte_t *pte_ptr(uint64 pgtbl_pa, uint64 va, int level)
{
	uint64 idx = VA_TO_VPN(va, level);
	pte_t *table = (pte_t *)pgtbl_pa;
	return &table[idx];
}
static pte_t *walk(uint64 pgtbl_pa, uint64 va, bool alloc)
{
}

/* --------------------------------------------------------------------------
 * 创建页表
 * -------------------------------------------------------------------------- */
pgtbl_t kvm_create(void)
{
	uint64 pa = pmem_alloc(true);
	if (!pa)
		panic("kvm_create: 无法分配页表页");
	return pa;
}
int kvm_map(pgtbl_t pgtbl, uint64 va, uint64 pa, uint64 size, int perm)
{
}
void kvm_unmap(pgtbl_t pgtbl, uint64 va, uint64 size)
{
	for (uint64 off = 0; off < size; off += PGSIZE) {
		pte_t *pte = walk(pgtbl, va + off, false);
		if (pte && (*pte & PTE_V)) {
			*pte = 0;
			/* 刷新该地址的 TLB, 否则 CPU 可能继续使用旧映射 */
			arch_tlb_flush_page(va + off);
		}
	}
}

/* --------------------------------------------------------------------------
 * 查询 va 对应的物理地址
 * -------------------------------------------------------------------------- */
uint64 kvm_translate(pgtbl_t pgtbl, uint64 va)
{
	pte_t *pte = walk(pgtbl, va & ~(PGSIZE - 1), false);
	if (!pte || !(*pte & PTE_V))
		return 0;
	return PTE_TO_PA(*pte) + (va & (PGSIZE - 1));
}
