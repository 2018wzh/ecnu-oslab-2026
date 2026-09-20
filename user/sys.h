#ifndef OSLAB_USER_SYS_H
#define OSLAB_USER_SYS_H
#include <uapi/syscall.h>
long syscall6(long number, long x0, long x1, long x2, long x3, long x4, long x5);
long print_str(const char *str);
long print_int(int value);
long brk(unsigned long top);
long mmap(unsigned long address, unsigned long len);
long munmap(unsigned long address, unsigned long len);
long getpid(void);
long fork(void);
long wait(int *status);
void exit(int status) __attribute__((noreturn));
long sleep(unsigned long ticks);
#endif
