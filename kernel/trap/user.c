#include <kernel/proc.h>
#include <kernel/syscall.h>
#include <uapi/syscall.h>
#include <kernel/arch.h>
#include <kernel/print.h>
#include <asm/csr.h>
/* TODO(lab-4): 关闭中断的入口中安装内核向量，确认来自 U-mode。
 * trampoline 已保存用户 PC/status；处理期间保全 frame 中的返回状态。
 * 时钟/外部中断复用 lab-3；其他无法处理的陷阱报告原因、PC、stval 后 panic。
 * 对 U-mode ecall，用 arch_syscall_decode 取调用号，在这里写最小分支：
 * SYS_hello 输出 "proczero: hello world!\n" 并返回 0，未知调用返回 -38。
 * 仅通过 arch_syscall_return 写返回值并推进 PC 一次；中断不推进 PC。
 * 最后 enter_user；不引入通用 syscall 分派或用户指针解引用。
 */
void user_trap(void) { panic("TODO(lab-4): user_trap"); }
/* TODO(lab-4): 关闭中断，填写 frame 的内核 satp、虚拟栈顶、user_trap 和 hartid。
 * 用户页表和 frame 有效且由当前进程独占；通过 arch_user_return 完成架构准备。
 */
void enter_user(void) { panic("TODO(lab-4): enter_user"); }
