/* 空闲页面的开头存放链表节点，分配后归调用者使用。 */
#include <kernel/mem.h>
#include <kernel/print.h>
#include <kernel/string.h>
#include <platform.h>
/* 两个独立的私有池；不能在模块外直接操作链表或计数。 */
static alloc_region_t kern_region __attribute__((unused));
static alloc_region_t user_region __attribute__((unused));
/* TODO(lab-2): 初始化仅执行一次，先于任何并发分配。
 * 从页对齐的 kernel_end 到 DRAM_BASE + DRAM_SIZE 划分两池，
 * 前 1024 页供内核，剩余供普通页面；填写边界、锁、计数和哨兵空闲链表。
 * 可自行提取私有辅助函数组织链表，不要求独立公开接口。
 */
void pmem_init(void) { panic("TODO(lab-2): pmem_init"); }
/* TODO(lab-2): 返回指定池的一个清零物理页；失败（包括耗尽）则 panic。
 * 持对应池锁摘链并更新计数；摘出的页面已独占，可在解锁后清零。
 */
void *pmem_alloc(bool kernel)
{ (void)kernel; panic("TODO(lab-2): pmem_alloc"); }
/* TODO(lab-2): 释放一个之前从指定池申请的物理页；失败则 panic。
 * 调用者停止使用页面后，持对应池锁归还链表并更新计数。
 */
void pmem_free(uint64 pa, bool kernel)
{ (void)pa; (void)kernel; panic("TODO(lab-2): pmem_free"); }
