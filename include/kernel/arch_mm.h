/* 架构相关的内存管理事实 (arch-api): 充当接缝, 提供 PGSIZE/PGSHIFT、
 * ARCH_PTE_PER_TABLE、ARCH_KERNEL_PTE_START 等编译期常量, 由 <asm/pgtable.h> 提供。 */
#ifndef __KERNEL_ARCH_MM_H__
#define __KERNEL_ARCH_MM_H__

#include <asm/pgtable.h>

// 页表项的类型, generic kernel 不应知道它是 uint64。
typedef pte_t arch_pte_t;

// 一级页表里的项数。
#define ARCH_PTE_PER_TABLE PTE_PER_TABLE

// 根页表里内核半部的起始下标: 复制内核页表只复制这一半,
// 连低半部一起复制会让所有进程共享用户空间的下一级页表, 破坏地址空间隔离。
#ifndef ARCH_KERNEL_PTE_START
#define ARCH_KERNEL_PTE_START (PTE_PER_TABLE / 2)
#endif

#endif /* __KERNEL_ARCH_MM_H__ */
