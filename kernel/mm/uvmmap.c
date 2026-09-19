// 用户地址空间管理: 每个进程一张独立页表实现隔离, 用户页面映射来自内核, 但内核映射
// 无 PTE_U 位用户态也访问不到; 本文件只调用 generic 的 kvm_map/kvm_unmap, 不关心 Sv39/PTE 位。
#include <kernel/types.h>
#include <kernel/mm.h>
#include <kernel/proc.h>
#include <kernel/print.h>
#include <kernel/string.h>
#include <kernel/arch.h>
#include <kernel/arch_mm.h>

/* 用户地址空间起始: 第一页不用, 这样"空指针解引用"会立刻触发缺页,
 * 而不是静默地访问到有效数据 —— 让 bug 尽早暴露。 */
#define USER_BASE PGSIZE
uint64 uvm_alloc(pgtbl_t pgtbl, uint64 va, uint64 size, int perm)
{
	va = ALIGN_DOWN(va, PGSIZE);
	size = ALIGN_UP(size, PGSIZE);

	for (uint64 off = 0; off < size; off += PGSIZE) {
		uint64 pa = pmem_alloc_user(true);
		if (!pa)
			return 0;   /* 内存耗尽 */
		if (kvm_map(pgtbl, va + off, pa, PGSIZE, perm) < 0) {
			pmem_free_user(pa);
			return 0;
		}
	}
	return va;
}
int uvm_copyout(pgtbl_t pgtbl, uint64 dst_va, uint64 src_kva, uint64 len)
{
	if (!pgtbl)
		return -1;

	while (len > 0) {
		/* 计算这一页内能拷贝多少字节 */
		uint64 pa = kvm_translate(pgtbl, dst_va);
		if (pa == 0) {
			printf("[uvm] copyout: 地址 0x%lx 未映射\n", dst_va);
			return -1;
		}

		uint64 page_off = dst_va & (PGSIZE - 1);
		uint64 n = MIN(len, PGSIZE - page_off);

		memmove((void *)pa, (void *)src_kva, n);

		dst_va  += n;
		src_kva += n;
		len     -= n;
	}
	return 0;
}

/* --------------------------------------------------------------------------
 * 从用户地址空间拷贝数据到内核 (copyin)
 * -------------------------------------------------------------------------- */
int uvm_copyin(pgtbl_t pgtbl, uint64 dst_kva, uint64 src_va, uint64 len)
{
	if (!pgtbl)
		return -1;

	while (len > 0) {
		uint64 pa = kvm_translate(pgtbl, src_va);
		if (pa == 0) {
			printf("[uvm] copyin: 地址 0x%lx 未映射\n", src_va);
			return -1;
		}

		uint64 page_off = src_va & (PGSIZE - 1);
		uint64 n = MIN(len, PGSIZE - page_off);

		memmove((void *)dst_kva, (void *)pa, n);

		dst_kva += n;
		src_va  += n;
		len     -= n;
	}
	return 0;
}
