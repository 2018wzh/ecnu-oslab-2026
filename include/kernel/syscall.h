#ifndef OSLAB_SYSCALL_H
#define OSLAB_SYSCALL_H
#include <kernel/trap.h>
typedef struct { uint64 number; uint64 args[6]; } syscall_args_t;
syscall_args_t arch_syscall_decode(const trapframe_t *frame);
void arch_syscall_return(trapframe_t *frame, long result);
/* 教师 ABI 适配；仅 ecall 路径调用 return，PC 在此推进一次。 */
#endif
