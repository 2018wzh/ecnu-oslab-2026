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
long sys_alloc_block(const syscall_args_t *call);
long sys_free_block(const syscall_args_t *call);
long sys_alloc_inode(const syscall_args_t *call);
long sys_free_inode(const syscall_args_t *call);
long sys_show_bitmap(const syscall_args_t *call);
long sys_get_block(const syscall_args_t *call);
long sys_read_block(const syscall_args_t *call);
long sys_write_block(const syscall_args_t *call);
long sys_put_block(const syscall_args_t *call);
long sys_show_buffer(const syscall_args_t *call);
long sys_flush_buffer(const syscall_args_t *call);
#endif
