#include <kernel/syscall.h>
#include <kernel/proc.h>
#include <kernel/print.h>
#include <uapi/syscall.h>
static long getpid_call(const syscall_args_t *call) { (void)call; return sys_getpid(); }
static long fork_call(const syscall_args_t *call) { (void)call; return sys_fork(); }
/* 教师分派：完整 64 位编号检查，0 与超范围号均不调用空指针。 */
static long (*const handlers[])(const syscall_args_t *) = {
    [SYS_BRK] = sys_brk, [SYS_MMAP] = sys_mmap, [SYS_MUNMAP] = sys_munmap,
    [SYS_PRINT_STR] = sys_print_str, [SYS_PRINT_INT] = sys_print_int,
    [SYS_GETPID] = getpid_call, [SYS_FORK] = fork_call,
    [SYS_WAIT] = sys_wait, [SYS_EXIT] = sys_exit, [SYS_SLEEP] = sys_sleep,
};
long syscall_dispatch(const syscall_args_t *call)
{
    if (call->number >= sizeof(handlers) / sizeof(handlers[0]) || !handlers[call->number]) {
        printf("unknown syscall %p pid=%d\n", (void *)call->number, myproc()->pid);
        panic("unknown syscall");
    }
    return handlers[call->number](call);
}
