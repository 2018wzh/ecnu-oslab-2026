// Sv39 页表格式 (架构相关)。
// Sv39 的 PTE 位布局由 RISC-V 规范规定, AArch64 完全不同 (4/3 级配置)。
// generic VM 算法只需知道"映射 4KB 页、权限是读写执行", 不应知道 PTE 的
// 物理页号在哪几位。
// Sv39: 虚拟地址 39 位 = VPN[2](9)+VPN[1](9)+VPN[0](9)+offset(12), 三级页表
// 每级 512 项每项 8 字节 (恰一个 4KB 页)。PTE: 63:54 rsv | 53:10 PPN | 9:8 RSW
// | D A G U X W R V。V=1 且 R/W/X 全 0 是指向下一级页表的指针, 否则是叶子。
#ifndef __ASM_PGTABLE_H__
#define __ASM_PGTABLE_H__

#include <kernel/types.h>
#include <kernel/perm.h>
#define PGSIZE  4096
#define PGSHIFT 12

/* 每级页表的项数: PGSIZE / sizeof(pte_t) = 4KB / 8B = 512 */
#define PTE_PER_TABLE (PGSIZE / 8)

/* 页表项类型 */
typedef uint64 pte_t;

/* Sv39 的地址位数与各级位移 */
#define VA_BITS_SV39   39
#define PTE_LEVELS     3
#define VA_SHIFT(level) (PGSHIFT + 9 * (level))

/* 从虚拟地址提取第 level 级页号 (9 位) */
#define VA_TO_VPN(va, level) ((((uint64)(va)) >> VA_SHIFT(level)) & 0x1FF)

/* ---- PTE 标志位 ---- */
#define PTE_V (1UL << 0)   /* 有效 */
#define PTE_R (1UL << 1)   /* 可读 */
#define PTE_W (1UL << 2)   /* 可写 */
#define PTE_X (1UL << 3)   /* 可执行 */
#define PTE_U (1UL << 4)   /* 用户态可访问 */
#define PTE_G (1UL << 5)   /* 全局映射 (不随 ASID 切换失效) */
#define PTE_A (1UL << 6)   /* 已访问 */
#define PTE_D (1UL << 7)   /* 已写过 */

/* 判断一个 PTE 是否指向下一级页表 */
#define PTE_IS_LEAF(pte) (((pte) & (PTE_R | PTE_W | PTE_X)) != 0)

/* ---- 物理地址 <-> PTE 编码 ---- */
/* PTE 中的物理页号从第 10 位开始 */
#define PA_TO_PTE(pa)  ((((uint64)(pa)) >> PGSHIFT) << 10)
#define PTE_TO_PA(pte) ((((uint64)(pte)) >> 10) << PGSHIFT)
#define ARCH_USER_STACK_TOP  0x80000000UL   /* 2^31, 落在 SV39 低半部内 */
#define ARCH_USER_STACK_SIZE (16 * PGSIZE)
#define ARCH_KERNEL_PTE_START 256

/* ---- satp 寄存器 ---- */
#define SATP_MODE_SV39 (8UL << 60)
#define MAKE_SATP(pgtbl_pa) (SATP_MODE_SV39 | (((uint64)(pgtbl_pa)) >> PGSHIFT))

/* ---- 权限翻译: 把 generic 的 PERM_* 翻译成 RISC-V 的 PTE 位 ----
 * 这个函数是 generic 与 arch 之间的"唯一翻译点"。
 * 其他架构只需要提供同名的 arch_perm_to_pte()。 */
static inline uint64 arch_perm_to_pte(int perm)
{
	uint64 pte = PTE_V;
	if (perm & PERM_R) pte |= PTE_R;
	if (perm & PERM_W) pte |= PTE_W;
	if (perm & PERM_X) pte |= PTE_X;
	if (perm & PERM_U) pte |= PTE_U;
	/* RISC-V 规定: 只要不是纯执行页, 都应设置 A/D 位
	 * (QEMU 会硬件置位, 但某些实现不置, 显式设置更保险)。
	 * VisionFive2 的 hart 需要显式 D 位才能写, 否则触发缺页异常 ——
	 * 这是 2025 版本里 PTE_MACHINE 那个宏想解决的问题,
	 * 现在它被正确地放在 arch 层而不是散落在 mem/type.h 里。 */
	if (perm & (PERM_R | PERM_W))
		pte |= PTE_A | PTE_D;
	return pte;
}

#endif /* __ASM_PGTABLE_H__ */
