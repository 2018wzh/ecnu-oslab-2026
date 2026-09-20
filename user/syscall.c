/* 用户库只表达系统调用语义，寄存器约定由架构层实现。 */
#include "sys.h"
#include <syscall_arch.h>
long syscall6(long number, long x0, long x1, long x2, long x3, long x4, long x5)
{ return arch_syscall6(number, x0, x1, x2, x3, x4, x5); }
long hello(void) { return syscall6(SYS_HELLO, 0, 0, 0, 0, 0, 0); }
