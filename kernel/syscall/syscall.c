#include <kernel/syscall.h>
#include <kernel/proc.h>
#include <kernel/print.h>
#include <uapi/syscall.h>
static long hello_call(const syscall_args_t *call) { (void)call; return sys_hello(); }
/* 教师函数表；检查完整 64 位调用号，不能先截断到小整数。 */
static long (*const handlers[])(const syscall_args_t *) = {
    [SYS_HELLO] = hello_call,
    [SYS_TEST_COPYIN] = sys_test_copyin,
    [SYS_TEST_COPYOUT] = sys_test_copyout,
    [SYS_TEST_COPYINSTR] = sys_test_copyinstr,
    [SYS_BRK] = sys_brk, [SYS_MMAP] = sys_mmap, [SYS_MUNMAP] = sys_munmap,
};
long syscall_dispatch(const syscall_args_t *call)
{
    if (call->number >= sizeof(handlers) / sizeof(handlers[0])) {
        printf("unknown syscall %p pid=%d\n", (void *)call->number, myproc()->pid);
        panic("unknown syscall");
    }
    return handlers[call->number](call);
}
