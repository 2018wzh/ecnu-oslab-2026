/* 空闲页面的开头存放链表节点，分配后归调用者使用。 */
#include <kernel/mem.h>
#include <kernel/print.h>
#include <kernel/string.h>
#include <platform.h>
alloc_region_t regions[2];
// TODO(lab-2): 从 kernel_end 起划分两池；前 1024 页供内核，剩余供普通页面。
void pmem_init(void) { panic("TODO(lab-2): pmem_init"); }
// TODO(lab-2): 把 [begin,end) 内每个对齐页面组织为空闲链表。
void build_free_list(alloc_region_t *region)
{ (void)region; panic("TODO(lab-2): build_free_list"); }
// TODO(lab-2): 持锁摘取页面、更新计数，清零后返回；耗尽返回 NULL。
void *pmem_alloc(bool kernel)
{ (void)kernel; panic("TODO(lab-2): pmem_alloc"); }
// TODO(lab-2): 检查对齐、归属和重复释放，持锁归还页面。
void pmem_free(uint64 pa, bool kernel)
{ (void)pa; (void)kernel; panic("TODO(lab-2): pmem_free"); }
