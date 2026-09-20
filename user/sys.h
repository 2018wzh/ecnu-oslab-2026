#ifndef OSLAB_USER_SYS_H
#define OSLAB_USER_SYS_H
#include <uapi/syscall.h>
long syscall6(long number, long x0, long x1, long x2, long x3, long x4, long x5);
long hello(void);
#endif
