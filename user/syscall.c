/* 用户库只表达系统调用语义，寄存器约定由架构层实现。 */
#include "sys.h"
long fork(void) { return syscall6(SYS_FORK, 0, 0, 0, 0, 0, 0); }
long wait(int *status) { return syscall6(SYS_WAIT, (long)status, 0, 0, 0, 0, 0); }
void exit(int status) { syscall6(SYS_EXIT, status, 0, 0, 0, 0, 0); for (;;) {} }
long sleep(unsigned long ticks) { return syscall6(SYS_SLEEP, ticks, 0, 0, 0, 0, 0); }
#include <syscall_arch.h>
long syscall6(long number, long x0, long x1, long x2, long x3, long x4, long x5)
{ return arch_syscall6(number, x0, x1, x2, x3, x4, x5); }
long print_str(const char *str) { return syscall6(SYS_PRINT_STR, (long)str, 0, 0, 0, 0, 0); }
long print_int(int value) { return syscall6(SYS_PRINT_INT, value, 0, 0, 0, 0, 0); }
long brk(unsigned long top) { return syscall6(SYS_BRK, top, 0, 0, 0, 0, 0); }
long mmap(unsigned long address, unsigned long len)
{ return syscall6(SYS_MMAP, address, len, 0, 0, 0, 0); }
long munmap(unsigned long address, unsigned long len)
{ return syscall6(SYS_MUNMAP, address, len, 0, 0, 0, 0); }
long getpid(void) { return syscall6(SYS_GETPID, 0, 0, 0, 0, 0, 0); }
