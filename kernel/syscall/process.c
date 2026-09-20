#include <kernel/syscall.h>
#include <kernel/print.h>
// TODO(lab-6): 返回当前进程号。
long sys_getpid(void) { panic("TODO(lab-6): sys_getpid"); }
// TODO(lab-6): 分别调用进程操作，参数转换及错误处理在此完成。
long sys_fork(void) { panic("TODO(lab-6): sys_fork"); }
long sys_exit(const syscall_args_t *call) { (void)call; panic("TODO(lab-6): sys_exit"); }
long sys_wait(const syscall_args_t *call) { (void)call; panic("TODO(lab-6): sys_wait"); }
long sys_sleep(const syscall_args_t *call) { (void)call; panic("TODO(lab-6): sys_sleep"); }
