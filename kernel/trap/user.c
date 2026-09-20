#include <kernel/proc.h>
#include <kernel/uvm.h>
#include <kernel/syscall.h>
#include <uapi/syscall.h>
#include <kernel/arch.h>
#include <kernel/print.h>
#include <asm/csr.h>
/* TODO(lab-4): 关闭中断的入口中安装内核向量，确认来自 U-mode。
 * trampoline 已保存用户 PC/status；处理期间保全 frame 中的返回状态。
 * 时钟/外部中断复用 lab-3；其他无法处理的陷阱报告原因、PC、stval 后 panic。
 * 对 U-mode ecall，用 arch_syscall_decode 解码，再调用 syscall_dispatch。
 * TODO(lab-5): 接入教师函数表；未知号由分派器报告调用号和 pid 后 panic。
 * 仅通过 arch_syscall_return 写返回值并推进 PC 一次；中断不推进 PC。
 * TODO(lab-5): 识别完整异常号 13/15，读取 stval，调用 uvm_stack_grow。
 * 合法增长后重试原指令，不推进 PC；非法栈缺页 panic。
 * 最后 enter_user；用户地址仅经复制接口访问。
 */
void user_trap(void) { panic("TODO(lab-4): user_trap"); }
/* TODO(lab-4): 关闭中断，填写 frame 的内核 satp、虚拟栈顶、user_trap 和 hartid。
 * 用户页表和 frame 有效且由当前进程独占；通过 arch_user_return 完成架构准备。
 */
void enter_user(void) { panic("TODO(lab-4): enter_user"); }
