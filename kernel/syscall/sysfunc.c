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
#include <kernel/block.h>
#include <uapi/syscall.h>


// helloworld: 内核打印固定字符串 (lab-4 唯一需要的系统调用)。
// lab-4 第一个用户进程只需证明"用户态->ecall->trap->syscall"链路通, 字符串由
// 内核 printf 输出, 不需要用户内存拷贝或 fd 表/文件抽象 (lab-9 才出现)。
int64 sys_write(int fd, uint64 buf, uint64 n)
{
	proc_t *p = myproc();
	if (!p || !p->pgtbl)
		return E_BADARG;
	if (n == 0)
		return 0;
	if (n > 64 * 1024)
		return E_BADARG;   /* 单次写入过大, 防止内存耗尽 */

	file_t *f = fd_get(p, (int)fd);
	if (!f)
		return E_NOFD;
	if (!f->writable)
		return E_BADARG;

	/* 控制台 / 设备: 直接输出字符。
	 * 用分块拷贝到内核缓冲区的方式, 避免长时间持有用户页表引用,
	 * 也避免一次分配过大内存。 */
	if (f->type == FD_CONSOLE || f->type == FD_DEVICE) {
		char kbuf[256];
		uint64 done = 0;
		while (done < n) {
			uint64 chunk = n - done;
			if (chunk > sizeof(kbuf))
				chunk = sizeof(kbuf);
			if (uvm_copyin(p->pgtbl, (uint64)kbuf, buf + done, chunk) < 0)
				return E_BADARG;
			for (uint64 i = 0; i < chunk; i++)
				console_putc(kbuf[i]);
			done += chunk;
		}
		return (int64)n;
	}

	/* 普通文件: 通过 inode 写入 (inode_write 内部处理用户地址校验) */
	if (f->type == FD_INODE) {
		int r = inode_write(f->ip, buf, (uint32)f->off, (uint32)n, 1);
		if (r > 0)
			f->off += (uint64)r;
		return (int64)r;
	}

	return E_BADARG;
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

/* --------------------------------------------------------------------------
 * 25 lab-9 补齐的目录/链接/元数据系统调用 (对齐 2025 的 22 个 syscall 集合)
 * -------------------------------------------------------------------------- */

/* 复制一个文件描述符 (新 fd 指向同一个 file 对象; 用 file_dup + fd_alloc) */
int64 sys_dup(int fd)
{
}

/* 取一个打开文件的元数据 (用 inode_stat) */
int64 sys_fstat(int fd)
{
}

/* 列出目录下的有效目录项 (目录 inode 里的 dirent) */
int64 sys_get_dentries(int fd)
{
}

/* 创建一个目录 (用 inode_create, type = 目录) */
int64 sys_mkdir(uint64 path_user)
{
}

/* 切换当前工作目录 (更新 proc->cwd) */
int64 sys_chdir(uint64 path_user)
{
}

/* 打印当前工作目录的绝对路径 */
int64 sys_print_cwd(void)
{
}

/* 建立硬链接 (用 dir_link + 增加 nlink) */
int64 sys_link(uint64 old_path_user, uint64 new_path_user)
{
}

/* 解除硬链接 (减少 nlink, 目录项移除) */
int64 sys_unlink(uint64 path_user)
{
}
