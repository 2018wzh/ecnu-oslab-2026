/* trapframe 与通用系统调用请求之间的转换。 */
#include <kernel/syscall.h>
#include <kernel/proc.h>
#include <kernel/arch.h>
#include <kernel/print.h>
#include <asm/csr.h>
syscall_args_t arch_syscall_decode(const trapframe_t *frame)
{
    syscall_args_t call = { .number = frame->x[17] };
    for (unsigned i = 0; i < 6; i++) call.args[i] = frame->x[10 + i];
    return call;
}
void arch_syscall_return(trapframe_t *frame, long result)
{
    frame->x[10] = (uint64)result;
    frame->epc += 4; /* RISC-V ecall 为固定 4 字节指令。 */
}
uint64 arch_kernel_satp(void) { return csr_read(satp); }
/* TODO(lab-4): 保持中断关闭，同步指令缓存，设置高地址 user_vector 为 stvec，
 * 写 sepc，准备 sstatus 的 SPP/SPIE/SIE，经高地址 user_return(TRAPFRAME, satp) 返回。
 * 学生负责这些准备；csr_read/write、switch/trampoline 汇编由教师提供。
 */
void arch_user_return(uint64 user_satp, uint64 pc)
{ (void)user_satp; (void)pc; panic("TODO(lab-4): arch_user_return"); }
