#include <kernel/proc.h>
#include <kernel/arch.h>
#include <kernel/print.h>
#include <platform.h>
proc_t *proczero;
static proc_t *current[NCPU];
/* 调用者关闭中断，且在使用返回指针期间不能迁移 CPU。 */
proc_t *myproc(void) { return current[arch_cpu_id()]; }
void set_current(proc_t *p) { current[arch_cpu_id()] = p; }
/* TODO(lab-4): 从内核池申请并清零根页表，映射 trampoline RX 和 frame RW。
 * 两者均不设置 U；trampoline 使用内核相同 VA/PA，映射成功后 frame_pa 由用户页表独占，uvm_destroy 释放。
 * 遵循 lab-2 的 vm_* 契约，分配耗尽或映射错误 panic。
 */
pgtbl_t proc_pgtbl_init(uint64 frame_pa)
{ (void)frame_pa; panic("TODO(lab-4): proc_pgtbl_init"); }
/* TODO(lab-6): 主核使用 proc_slot_alloc 创建 proczero（指向数组的一槽）。
 * 保留 lab-4 镜像/用户栈初始化：代码数据 USER_ENTRY RWXU，用户栈 RWU，最低页不映射；
 * 普通池申请并清零镜像页和用户栈页，复制镜像内容；镜像 <= 一页，heap_top=0x2000、ustack_npage=1，mmap=NULL，设置用户 PC/SP。
 * 槽分配已准备独立 frame/页表、槽索引对应的常驻 kstack 及 first_return context。
 * 发布 RUNNABLE 并释放进程锁后返回；不得直接切换或发布 current。
 */
void proc_make_first(void) { panic("TODO(lab-6): proc_make_first"); }
