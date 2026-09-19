/* 用户态系统调用封装 (U-mode 库): 用户程序经它访问系统调用。
 * 调用号来自 uapi/syscall.h 保证与内核一致; "哪条指令、号放哪个寄存器"是架构事实,
 * 在 syscall_arch.h, 本文件只写"哪个调用号配哪几个参数"。 */
#include <uapi/syscall.h>
#include "syscall_arch.h"

// 进程控制。
void exit(int status)
{
	__syscall1(SYS_EXIT, status);
	// exit 不应返回; 若返回用死循环兜底, 避免执行到后面随机代码。
	for (;;)
		;
}

long fork(void)
{
	return __syscall0(SYS_FORK);
}

long wait(int *status)
{
	return __syscall1(SYS_WAIT, (long)status);
}

long getpid(void)
{
	return __syscall0(SYS_GETPID);
}

long exec(const char *path)
{
	// 第二个参数 (argv) 目前传 0: 本阶段内核还不支持传参。
	return __syscall2(SYS_EXEC, (long)path, 0);
}

// 输入输出。
long write(int fd, const void *buf, unsigned long n)
{
	return __syscall3(SYS_WRITE, fd, (long)buf, (long)n);
}

long read(int fd, void *buf, unsigned long n)
{
	return __syscall3(SYS_READ, fd, (long)buf, (long)n);
}

// 文件。
long open(const char *path, int flags)
{
	return __syscall2(SYS_OPEN, (long)path, flags);
}

long close(int fd)
{
	return __syscall1(SYS_CLOSE, fd);
}

long lseek(int fd, long offset, int whence)
{
	return __syscall3(SYS_LSEEK, fd, offset, whence);
}
