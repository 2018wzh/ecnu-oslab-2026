// 物理页分配器: 空闲页链表把空闲页自身当作链表节点,
// 分内核区与用户区两段管理, 与架构/平台无关。

#include <kernel/types.h>
#include <kernel/string.h>
#include <kernel/mm.h>
#include <kernel/print.h>
#include <platform.h>
#include <kernel/arch_mm.h>

/* 可分配区域的终点: 由平台内存大小决定 */
#define ALLOC_END ((uint64)(PLAT_DRAM_BASE + PLAT_DRAM_SIZE))

/* 内核保留的页数。
 * 内核自身需要一些页面来做页表、进程结构等。
 * 这里简单地"先切一刀", 前 KERN_PAGES 页归内核, 其余归用户。
 * 真正的内核会用更细的分配器 (slab 等), 教学版本用这个简化模型。 */
#define KERN_PAGES 4096

static mem_region_t kernel_region;
static mem_region_t user_region;
static void region_init(mem_region_t *r, uint64 begin, uint64 end)
{
	spinlock_init(&r->lk, "pmem");
	r->begin = ALIGN_UP(begin, PGSIZE);
	r->end = ALIGN_DOWN(end, PGSIZE);
	r->free_pages = 0;
	r->free_list = NULL;

	for (uint64 p = r->end - PGSIZE; p >= r->begin; p -= PGSIZE) {
		page_node_t *node = (page_node_t *)p;
		node->next = r->free_list;
		r->free_list = node;
		r->free_pages++;
		if (p == r->begin)
			break;   /* 防止 uint64 下溢 */
	}
}

void pmem_init(void)
{
}

/* --------------------------------------------------------------------------
 * 从指定区域分配一页
 * -------------------------------------------------------------------------- */

static uint64 region_alloc(mem_region_t *r, bool zero)
{
}

static void region_free(mem_region_t *r, uint64 pa)
{
}

/* --------------------------------------------------------------------------
 * 对外接口
 * -------------------------------------------------------------------------- */

/* 分配一个内核页 (用于页表、内核数据结构) */
uint64 pmem_alloc(bool zero)
{
	return region_alloc(&kernel_region, zero);
}

/* 分配一个用户页 */
uint64 pmem_alloc_user(bool zero)
{
	return region_alloc(&user_region, zero);
}

/* 释放: 根据地址落在哪个区域自动判断, 调用者不需要记住来源。
 * 这样接口更不容易用错。 */
void pmem_free(uint64 pa)
{
	if (pa >= kernel_region.begin && pa < kernel_region.end)
		region_free(&kernel_region, pa);
	else if (pa >= user_region.begin && pa < user_region.end)
		region_free(&user_region, pa);
	else
		panic("pmem_free: 地址 0x%lx 不属于任何可分配区域", pa);
}

/* 释放用户页 (显式版本, 便于调试时确认配对正确) */
void pmem_free_user(uint64 pa)
{
	region_free(&user_region, pa);
}

void pmem_stat(uint32 *kernel_free, uint32 *user_free)
{
	if (kernel_free) *kernel_free = kernel_region.free_pages;
	if (user_free)   *user_free = user_region.free_pages;
}
