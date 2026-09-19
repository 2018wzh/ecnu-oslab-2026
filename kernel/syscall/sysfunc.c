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

/* --------------------------------------------------------------------------
 * helloworld (lab-4 起): 内核打印固定字符串, 不经 fd 表。
 * -------------------------------------------------------------------------- */
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
	/* fd 表与文件抽象属 lab-9; 前面阶段用户输出走 SYS_HELLOWORLD。 */
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
	proc_t *p = myproc();
	if (!p)
		return E_BADARG;
	proc_exit((int)status);
	return E_OK;   /* 不会到达 */
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


	// 把整个可执行文件读进内核内存。不在读取时直接映射: 解析 ELF 要先看头部
	// 才知道映射哪些段, 先整读最简单可靠 (教学内核不必流式加载; 真实内核用
	// mmap 按需映射支持几百 MB 文件)。
int64 sys_brk(uint64 new_brk)
{
}
int64 sys_mmap(uint64 len)
{
}
int64 sys_munmap(uint64 addr, uint64 len)
{
}

/* 睡 n 个 tick (对齐 2025 lab-6: proc_sleep + 时钟唤醒)。 */
int64 sys_sleep(int n)
{
}
