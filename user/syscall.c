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
long alloc_block(void) { return syscall6(SYS_ALLOC_BLOCK, 0, 0, 0, 0, 0, 0); }
long free_block(unsigned int number) { return syscall6(SYS_FREE_BLOCK, number, 0, 0, 0, 0, 0); }
long alloc_inode(void) { return syscall6(SYS_ALLOC_INODE, 0, 0, 0, 0, 0, 0); }
long free_inode(unsigned int number) { return syscall6(SYS_FREE_INODE, number, 0, 0, 0, 0, 0); }
long show_bitmap(unsigned int inode) { return syscall6(SYS_SHOW_BITMAP, inode, 0, 0, 0, 0, 0); }
long get_block(unsigned int block) { return syscall6(SYS_GET_BLOCK, block, 0, 0, 0, 0, 0); }
long read_block(unsigned long token, void *data) { return syscall6(SYS_READ_BLOCK, token, (long)data, 0, 0, 0, 0); }
long write_block(unsigned long token, const void *data) { return syscall6(SYS_WRITE_BLOCK, token, (long)data, 0, 0, 0, 0); }
long put_block(unsigned long token) { return syscall6(SYS_PUT_BLOCK, token, 0, 0, 0, 0, 0); }
long show_buffer(void) { return syscall6(SYS_SHOW_BUFFER, 0, 0, 0, 0, 0, 0); }
long flush_buffer(unsigned int count) { return syscall6(SYS_FLUSH_BUFFER, count, 0, 0, 0, 0, 0); }
