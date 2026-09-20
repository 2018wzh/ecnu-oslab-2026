#ifndef OSLAB_SYSCALL_H
#define OSLAB_SYSCALL_H
#include <kernel/trap.h>
typedef struct { uint64 number; uint64 args[6]; } syscall_args_t;
syscall_args_t arch_syscall_decode(const trapframe_t *frame);
void arch_syscall_return(trapframe_t *frame, long result);
/* 用户 trap 保存原调用号；成功 exec 使用已替换的新 frame，不推进新 PC。 */
void arch_syscall_finish(trapframe_t *frame, uint64 number, long result);
long syscall_dispatch(const syscall_args_t *call);
/* 教师声明与参数辅助；路径缓冲 128 字节，缺 NUL 返回 -1。 */
int arg_path(const syscall_args_t *call, unsigned n, char out[128]);
long sys_brk(const syscall_args_t *call);
long sys_mmap(const syscall_args_t *call);
long sys_munmap(const syscall_args_t *call);
long sys_fork(void);
long sys_wait(const syscall_args_t *call);
long sys_exit(const syscall_args_t *call);
long sys_sleep(const syscall_args_t *call);
long sys_getpid(void);
long sys_exec(const syscall_args_t *call);
long sys_open(const syscall_args_t *call);
long sys_close(const syscall_args_t *call);
long sys_read(const syscall_args_t *call);
long sys_write(const syscall_args_t *call);
long sys_lseek(const syscall_args_t *call);
long sys_dup(const syscall_args_t *call);
long sys_fstat(const syscall_args_t *call);
long sys_get_dentries(const syscall_args_t *call);
long sys_mkdir(const syscall_args_t *call);
long sys_chdir(const syscall_args_t *call);
long sys_print_cwd(const syscall_args_t *call);
long sys_link(const syscall_args_t *call);
long sys_unlink(const syscall_args_t *call);
#endif
