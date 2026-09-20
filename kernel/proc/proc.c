#include <kernel/proc.h>
#include <kernel/arch.h>
#include <kernel/print.h>
#include <platform.h>
proc_t proczero;
static proc_t *current[NCPU];
/* 每核启动执行流的 context 存储；由 proc_make_first 在切换时使用。 */
context_t boot_context[NCPU];
/* 调用者关闭中断，且在使用返回指针期间不能迁移 CPU。 */
proc_t *myproc(void) { return current[arch_cpu_id()]; }
void set_current(proc_t *p) { current[arch_cpu_id()] = p; }
/* TODO(lab-4): 从内核池申请并清零根页表，映射 trampoline RX 和 frame RW。
 * 两者均不设置 U；trampoline 使用内核相同 VA/PA，映射成功后 frame_pa 由用户页表独占，uvm_destroy 释放。
 * 遵循 lab-2 的 vm_* 契约，分配耗尽或映射错误 panic。
 */
pgtbl_t proc_pgtbl_init(uint64 frame_pa)
{ (void)frame_pa; panic("TODO(lab-4): proc_pgtbl_init"); }
/* TODO(lab-4): 主核完成内存与中断初始化后创建唯一首进程。
 * 初始化 pid/state，申请并清零内核池 frame，调用 proc_pgtbl_init。
 * 普通池申请并清零镜像页和用户栈页；检查镜像不超过一页，复制平坦镜像。
 * 映射 USER_ENTRY 处代码数据 RWXU、用户栈 RWU；最低页不映射。
 * 设置 heap_top=0x2000、ustack_npage=1、用户 PC/SP。
 * TODO(lab-5): 初始化 mmap=NULL；节点池在创建首进程前初始化。
 * kstack=KSTACK(0)，物理页已由 kvm_init 分配映射；不能重复分配。
 * 准备 context.ra=enter_user、context.sp=栈顶（16 字节对齐）。
 * 关闭中断、发布 current，将本核启动 context 保存到 boot_context，调用 arch_switch。
 * 不引入调度循环；不发布半初始化对象。kstack 是 VA，回收须查 PTE 得 PA。
 */
void proc_make_first(void) { panic("TODO(lab-4): proc_make_first"); }
