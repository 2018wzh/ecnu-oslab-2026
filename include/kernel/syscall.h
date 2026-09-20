#ifndef OSLAB_SYSCALL_H
#define OSLAB_SYSCALL_H
#include <kernel/trap.h>
typedef struct { uint64 number; uint64 args[6]; } syscall_args_t;
syscall_args_t arch_syscall_decode(const trapframe_t *frame);
void arch_syscall_return(trapframe_t *frame, long result);
long syscall_dispatch(const syscall_args_t *call);
long sys_print_str(const syscall_args_t *call);
long sys_print_int(const syscall_args_t *call);
long sys_mmap(const syscall_args_t *call);
long sys_munmap(const syscall_args_t *call);
long sys_brk(const syscall_args_t *call);
long sys_getpid(void);
long sys_fork(void);
long sys_exit(const syscall_args_t *call);
long sys_wait(const syscall_args_t *call);
long sys_sleep(const syscall_args_t *call);
#endif
