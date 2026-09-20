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
/* 测试专用：原样回传令牌，不解引用、不伪造、不重复归还；持有期间禁止 fork/exit。 */
long alloc_block(void);
long free_block(unsigned int number);
long alloc_inode(void);
long free_inode(unsigned int number);
long show_bitmap(unsigned int inode);
long get_block(unsigned int block);
long read_block(unsigned long token, void *data);
long write_block(unsigned long token, const void *data);
long put_block(unsigned long token);
long show_buffer(void);
long flush_buffer(unsigned int count);
#endif
