#ifndef OSLAB_MEM_H
#define OSLAB_MEM_H
#include <kernel/lock.h>
#define PAGE_SIZE 4096UL
#define KERNEL_PAGES 1024
/* 仅使用 Sv39 低半区，VA_MAX 为排他上界。每级 4096/8=512 项。 */
#define VA_MAX (1UL << 38)
/* Sv39：VA 的 VPN[2]/VPN[1]/VPN[0]/offset 分别占 9/9/9/12 位。
 * 一页存 4096/8=512 个 PTE。PTE：高 10 位本章清零、44 位 PPN、
 * 2 位 RSW、D/A/G/U/X/W/R/V；V 有效，U 用户，G 全局，A 已访问，D 已写入。
 * V=1 且 R/W/X=0 是指向下一级的非叶项，不是页表内存不可读写。
 * W=1 要求 R=1；本章只在 level 0 建立 4KiB 叶项。
 * satp：MODE[63:60]=8，ASID[59:44]=0，PPN[43:0]=根物理地址>>12。
 * ASID 是地址空间标识，不是 Flash 刷新；切换后须刷新本核翻译缓存。
 */
#define PTE_V 1UL
#define PTE_R 2UL
#define PTE_W 4UL
#define PTE_X 8UL
#define PTE_U 16UL
#define PTE_G 32UL
#define PTE_A 64UL
#define PTE_D 128UL
#define PA_TO_PTE(pa) (((uint64)(pa) >> 12) << 10)
#define PTE_TO_PA(pte) (((uint64)(pte) >> 10) << 12)
#define VPN(va, level) (((va) >> (12 + 9 * (level))) & 511)
typedef uint64 pte_t;
typedef pte_t *pgtbl_t;
typedef struct page_node { struct page_node *next; } page_node_t;
/* 每个空闲页的首个字存 next，分配后整页交给调用者；不另建节点数组。 */
typedef struct {
    uint64 begin, end;      /* 页对齐的半开区间，初始化发布后不变。 */
    spinlock_t lk;          /* 保护 allocable、list_head 及空闲页的 next 链接。 */
    uint32 allocable;       /* 当前可分配页数。 */
    page_node_t list_head;  /* 常驻哨兵，不是可分配页面；next == NULL 表示空。 */
} alloc_region_t;
extern pgtbl_t kernel_pgtbl;
extern char kernel_end[], text_end[], rodata_end[];
void pmem_init(void);
void *pmem_alloc(bool kernel);
void pmem_free(uint64 pa, bool kernel);
pte_t *vm_getpte(pgtbl_t root, uint64 va, bool alloc);
void vm_mappages(pgtbl_t root, uint64 va, uint64 pa, uint64 len, uint64 perm);
void vm_unmappages(pgtbl_t root, uint64 va, uint64 len, bool free_pages);
void kvm_init(void);
void kvm_inithart(void);
void vm_print(pgtbl_t root);
/* TODO(lab-7): 独立内核 VA 转 PA；查询 kernel_pgtbl，验证叶项，叠加页内偏移。 */
uint64 kvm_translate(uint64 va);
#endif
