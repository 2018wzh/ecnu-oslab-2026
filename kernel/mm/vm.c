// 内核地址空间管理: 物理内存直接映射到固定虚拟区间 (虚拟 = 物理 + KERNEL_MAP_OFFSET)。

#include <kernel/types.h>
#include <kernel/string.h>
#include <kernel/mm.h>
#include <kernel/print.h>
#include <kernel/arch.h>
#include <kernel/platform.h>
#include <platform.h>
#include <kernel/arch_mm.h>

/* 全局内核页表 */
static pgtbl_t g_kernel_pgtbl;

/* 内核直接映射区的偏移量。
 * 0 表示恒等映射 (虚拟地址 == 物理地址)。
 * 这样做的原因: 打开分页的那一刻, PC 还在执行下一条指令,
 * 如果映射不是恒等的, 必须保证"切换 satp 的那段代码"在两套映射下
 * 虚拟地址相同, 否则会立刻崩。恒等映射让这个问题消失, 便于教学。
 * 真实内核使用 trampoline 技巧解决这个问题。 */
#define KERNEL_MAP_OFFSET 0UL

pgtbl_t kvm_global(void)
{
	return g_kernel_pgtbl;
}
void kvm_init(void)
{
}
void kvm_init_hart(void)
{
	arch_mmu_activate(g_kernel_pgtbl);


	// 用 SBI 控制台 (不经页表) 先确认执行流还在, 再用 printf 验证 MMIO 映射对不对,
	// 从而把"分页出错"和"设备映射出错"两类问题分开排查。
	// 启动早期控制台见 include/kernel/arch.h, 属 ISA 相关功能
	/* 从核也会调用本函数 (satp 是每个 hart 自己的), 但那一行只需要打印一次。 */
	if (arch_cpu_is_boot_hart())
		arch_early_puts("[kvm] 页表已切换 (早期控制台)\n");
}
pgtbl_t kvm_copy_kernel(pgtbl_t kernel_pgtbl)
{
	pgtbl_t new = kvm_create();
	if (!new)
		return 0;

	/* ---- 1. 内核半部: 共享下级页表 ----
	 * 内核映射的 PTE 没有 PTE_U 位, 所以虽然每个进程的页表里都有
	 * 内核代码, 用户态也访问不到 —— 这就是"内核映射在每个进程
	 * 页表里, 但用户态访问不到"的机制。
	 * 它们下级那几张表只被内核映射使用, 不会被 uvm_alloc 改动,
	 * 所以共享是安全的。 */
	arch_pte_t *src = (arch_pte_t *)kernel_pgtbl;
	arch_pte_t *dst = (arch_pte_t *)new;
	for (int i = ARCH_KERNEL_PTE_START; i < ARCH_PTE_PER_TABLE; i++)
		dst[i] = src[i];
	for (int i = 0; i < ARCH_KERNEL_PTE_START; i++) {
		if (!(src[i] & PTE_V))
			continue;   /* 内核在这里没有映射: 留给本进程自己建 */

		pgtbl_t l1 = kvm_create();
		arch_pte_t *s1 = (arch_pte_t *)PTE_TO_PA(src[i]);
		arch_pte_t *d1 = (arch_pte_t *)l1;
		for (int j = 0; j < ARCH_PTE_PER_TABLE; j++)
			d1[j] = s1[j];
		dst[i] = PA_TO_PTE(l1) | PTE_V;
	}

	return new;
}
