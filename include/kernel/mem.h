#ifndef OSLAB_MEM_H
#define OSLAB_MEM_H
#include <kernel/lock.h>
#define PAGE_SIZE 4096UL
#define KERNEL_PAGES 1024
#define VA_MAX (1UL << 38)
#define PTE_V 1UL
#define PTE_R 2UL
#define PTE_W 4UL
#define PTE_X 8UL
#define PTE_U 16UL
#define PTE_A 64UL
#define PTE_D 128UL
#define PA_TO_PTE(pa) (((uint64)(pa) >> 12) << 10)
#define PTE_TO_PA(pte) (((uint64)(pte) >> 10) << 12)
#define VPN(va, level) (((va) >> (12 + 9 * (level))) & 511)
typedef uint64 pte_t;
typedef pte_t *pgtbl_t;
typedef struct page_node { struct page_node *next; } page_node_t;
typedef struct {
    uint64 begin, end;
    spinlock_t lock;
    uint64 free_count;
    page_node_t *head;
} alloc_region_t;
extern alloc_region_t regions[2]; /* 0：内核池，1：普通页面池。 */
extern pgtbl_t kernel_pgtbl;
extern char kernel_end[], text_end[], rodata_end[];
void pmem_init(void);
void build_free_list(alloc_region_t *region);
void *pmem_alloc(bool kernel);
void pmem_free(uint64 pa, bool kernel);
pte_t *walk(pgtbl_t root, uint64 va, bool alloc);
int map(pgtbl_t root, uint64 va, uint64 pa, uint64 len, uint64 perm);
void unmap(pgtbl_t root, uint64 va, uint64 len, bool free_pages);
void kvm_init(void);
void kvm_inithart(void);
void vm_print(pgtbl_t root);
#endif
