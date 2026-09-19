#ifndef __USER_HELP_H__
#define __USER_HELP_H__

/* 用户态"小 libc"的声明: 用户程序无 libc, 这里声明架构无关的
 * 字符串/输出/读一行辅助函数; 系统调用发起在 syscall_arch.h, 调用号在 uapi/syscall.h。 */

#include <uapi/syscall.h>

// 系统调用封装 (实现在 user/syscall.c)。
long write(int fd, const void *buf, unsigned long n);
long read(int fd, void *buf, unsigned long n);
long open(const char *path, int flags);
long close(int fd);
long lseek(int fd, long offset, int whence);
long fork(void);
long wait(int *status);
long getpid(void);
long exec(const char *path);
void exit(int status);

// 辅助函数 (实现在 user/help.c)。
unsigned long ustrlen(const char *s);
void uputs(const char *s);
void uputint(long v);
void uputhex(unsigned long v);
void uputc(char c);
void uprintf(const char *fmt, ...);

// 从标准输入读一行到 buf (最多 len-1 字符, 末尾补 '\0'); 返回实际读到的字符数, 0 表示暂无数据。
unsigned long ugets(char *buf, unsigned long len);

#endif /* __USER_HELP_H__ */
