// 具体系统调用的实现 (generic kernel)。
// 每个系统调用必须做三件事: 校验参数 (用户指针/长度/fd 不可信)、执行操作
// (经 fd 表找 file 对象再调方法)、返回错误 (负数错误码而非内核崩溃)。
// 黄金法则: 绝不解引用用户指针, 一切用户内存访问都要经 uvm_copyin/copyout/
// copyin_str 做页表校验, 否则传内核地址即可提权。
#include <kernel/types.h>
#include <kernel/print.h>
#include <kernel/string.h>
#include <kernel/mm.h>
#include <kernel/proc.h>
#include <kernel/fs.h>
#include <uapi/syscall.h>
int64 sys_helloworld(void)
{
	printf("proczero: hello world\n");
	return 0;
}

/* --------------------------------------------------------------------------
 * write: fd -> file -> (console 或 inode)
 * -------------------------------------------------------------------------- */

int64 sys_write(int fd, uint64 buf, uint64 n)
{
	/* 文件描述符与文件抽象在 lab-9 才出现。lab-4 的用户进程输出走
	 * SYS_HELLOWORLD (内核打印固定字符串), 不需要 write。 */
	(void)fd; (void)buf; (void)n;
	return E_NOSYS;
}

/* --------------------------------------------------------------------------
 * read: 从文件或控制台读取
 * -------------------------------------------------------------------------- */
int64 sys_read(int fd, uint64 buf, uint64 n)
{
}

/* --------------------------------------------------------------------------
 * open: 按路径打开文件
 * -------------------------------------------------------------------------- */

int64 sys_open(uint64 path_user, uint64 flags, uint64 mode)
{
}

int64 sys_close(int fd)
{
}

/* --------------------------------------------------------------------------
 * lseek: 移动读写位置
 * -------------------------------------------------------------------------- */
int64 sys_lseek(int fd, uint64 offset, uint64 whence)
{
}

/* --------------------------------------------------------------------------
 * getpid
 * -------------------------------------------------------------------------- */
int64 sys_getpid(void)
{
	proc_t *p = myproc();
	return p ? (int64)p->pid : -1;
}
int64 sys_exit(int status)
{

	// 释放进程资源。顺序: 先关文件, 再释放地址空间, 最后标 ZOMBIE 并让出 CPU。
	// 若先让出 CPU, 后面的清理代码永远不会执行。
	(void)status;
	for (;;)
		asm volatile("wfi");
}

int64 sys_fork(void)
{
}

int64 sys_wait(uint64 status_user)
{
}
int64 sys_exec(uint64 path_user, uint64 argv_user)
{
}
