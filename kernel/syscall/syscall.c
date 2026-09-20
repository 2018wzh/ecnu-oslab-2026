#include <kernel/syscall.h>
#include <kernel/proc.h>
#include <kernel/uvm.h>
#include <kernel/print.h>
#include <uapi/syscall.h>
static long getpid_call(const syscall_args_t *c) { (void)c; return sys_getpid(); }
static long fork_call(const syscall_args_t *c) { (void)c; return sys_fork(); }
/* 教师前八项；学生补齐 9～22，全部声明与编号已经给出。 */
static long (*const handlers[SYS_MAX_NUM + 1])(const syscall_args_t *) = {
    [SYS_BRK] = sys_brk, [SYS_MMAP] = sys_mmap, [SYS_MUNMAP] = sys_munmap,
    [SYS_FORK] = fork_call, [SYS_WAIT] = sys_wait, [SYS_EXIT] = sys_exit,
    [SYS_SLEEP] = sys_sleep, [SYS_GETPID] = getpid_call,
    // TODO(lab-9): 9～22 分派接线。
};
long syscall_dispatch(const syscall_args_t *call) {
    if (call->number > SYS_MAX_NUM || !handlers[call->number]) {
        printf("unknown syscall %p pid=%d\n", (void *)call->number, myproc()->pid);
        panic("unknown syscall");
    }
    return handlers[call->number](call);
}
int arg_path(const syscall_args_t *call, unsigned n, char out[128]) {
    if (n >= 6) return -1;
    copy_str_from_user(myproc(), out, call->args[n], PATH_BYTES);
    for (unsigned i = 0; i < PATH_BYTES; ++i) if (!out[i]) return 0;
    return -1;
}
