// 内核与用户地址空间之间的数据搬运: 用户指针不可信, 每次访问须先经页表校验,
// 不能直接解引用否则可能缺页崩溃或读写内核内存。映射页面见 uvmmap.c。
#include <kernel/types.h>
#include <kernel/mm.h>
#include <kernel/proc.h>
#include <kernel/print.h>
#include <kernel/string.h>
#include <kernel/arch_mm.h>

/* 用户地址空间起始 (与 uvmmap.c 保持一致: 第 0 页故意不映射) */
#define USER_BASE PGSIZE
int uvm_copyin_str(pgtbl_t pgtbl, char *dst, uint64 src_va, uint64 max)
{
}
void uvm_free(pgtbl_t pgtbl, uint64 sz)
{
}

/* --------------------------------------------------------------------------
 * 用户堆/栈/mmab 管理 (对齐 2025 lab-5 任务 2-5)
 *
 * 这些是本阶段的任务函数 (空体)。2025 里它们分别对应:
 *   任务2: sys_brk / uvm_heap_grow / uvm_heap_ungrow (用户堆手动管理)
 *          + uvm_ustack_grow (用户栈自动增长, 缺页时调用)
 *   任务3: mmap_region 仓库管理
 *   任务4: uvm_mmap / uvm_munmap
 *   任务5: uvm_destroy_pgtbl / uvm_copy_pgtbl (页表销毁与复制)
 * -------------------------------------------------------------------------- */

/* 增加堆顶到 cur_heap_top+len; 需要时为堆分配并映射新的物理页 */
uint64 uvm_heap_grow(pgtbl_t pgtbl, uint64 cur_heap_top, uint32 len)
{
}

/* 减少堆顶到 cur_heap_top-len; 需要时回收超出部分的物理页 */
uint64 uvm_heap_ungrow(pgtbl_t pgtbl, uint64 cur_heap_top, uint32 len)
{
}

uint64 uvm_ustack_grow(pgtbl_t pgtbl, uint64 old_ustack_npage, uint64 fault_addr)
{
}

mmap_region_t *uvm_mmap(uint64 begin, uint32 npages, int perm)
{
}

void uvm_munmap(uint64 begin, uint32 npages)
{
}

void uvm_destroy_pgtbl(pgtbl_t pgtbl)
{
}

void uvm_copy_pgtbl(pgtbl_t old, pgtbl_t new, uint64 heap_top, uint64 ustack_npage, mmap_region_t *mmap)
{
}
