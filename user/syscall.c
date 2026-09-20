#include "help.h"
#include "sys.h"

/*
    用户堆空间伸缩
    成功返回new_heap_top, 失败返回-1
*/
uint64 sys_brk(uint64 new_heap_top)
{
	return syscall6(SYS_BRK, (long)(new_heap_top), 0, 0, 0, 0, 0);
}

/*
    增加一段内存映射
    成功返回映射空间的起始地址, 失败返回-1
*/
uint64 sys_mmap(uint64 start, uint32 len)
{
	return syscall6(SYS_MMAP, (long)(start), (long)(len), 0, 0, 0, 0);
}

/*
    解除一段内存映射
    成功返回0 失败返回-1
*/
uint64 sys_munmap(uint64 start, uint32 len)
{
	return syscall6(SYS_MUNMAP, (long)(start), (long)(len), 0, 0, 0, 0);
}

/*
    进程复制
    返回子进程的pid
*/
uint32 sys_fork()
{
	return syscall6(SYS_FORK, 0, 0, 0, 0, 0, 0);
}

/*
    等待子进程退出
    成功返回子进程的pid, 失败返回-1
*/
uint32 sys_wait(uint32 *exit_state)
{
	return syscall6(SYS_WAIT, (long)(exit_state), 0, 0, 0, 0, 0);
}

/*
	进程退出
	不返回
*/
void sys_exit(uint32 exit_state)
{
	syscall6(SYS_EXIT, (long)(exit_state), 0, 0, 0, 0, 0);
    for (;;) {}
}

/*
	让进程睡眠一段时间(每个tick大约0.1秒)
	成功返回0
*/
uint32 sys_sleep(uint32 ntick)
{
	return syscall6(SYS_SLEEP, (long)(ntick), 0, 0, 0, 0, 0);
}

/*
	返回当前进程的pid
*/
uint32 sys_getpid()
{
	return syscall6(SYS_GETPID, 0, 0, 0, 0, 0, 0);
}

/*
    执行ELF文件以替换当前进程的内容
    成功返回argc, 失败返回-1
*/
uint32 sys_exec(char *path, char **argv)
{
	return syscall6(SYS_EXEC, (long)(path), (long)(argv), 0, 0, 0, 0);
}

/*
	打开或创建文件
	成功返回fd, 失败返回-1
*/
uint32 sys_open(char *path, uint32 open_mode)
{
	return syscall6(SYS_OPEN, (long)(path), (long)(open_mode), 0, 0, 0, 0);
}

/*
	关闭文件
	成功返回0, 失败返回-1
*/
uint32 sys_close(uint32 fd)
{
	return syscall6(SYS_CLOSE, (long)(fd), 0, 0, 0, 0, 0);
}

/*
	读取文件内容
	成功返回读到的字节数, 失败返回0
*/
uint32 sys_read(uint32 fd, uint32 len, void *addr)
{
	return syscall6(SYS_READ, (long)(fd), (long)(len), (long)(addr), 0, 0, 0);
}

/*
	写入文件内容
	成功返回写入的字节数, 失败返回0
*/
uint32 sys_write(uint32 fd, uint32 len, void *addr)
{
	return syscall6(SYS_WRITE, (long)(fd), (long)(len), (long)(addr), 0, 0, 0);
}

/*
	调整读写指针的位置
	成功返回新的偏移量, 失败返回-1
*/
uint32 sys_lseek(uint32 fd, uint32 offset, uint32 flag)
{
	return syscall6(SYS_LSEEK, (long)(fd), (long)(offset), (long)(flag), 0, 0, 0);
}

/*
    复制文件控制权
    成功返回new_fd, 失败返回-1
*/
uint32 sys_dup(uint32 fd)
{
	return syscall6(SYS_DUP, (long)(fd), 0, 0, 0, 0, 0);
}

/*
	获取文件信息
	成功返回0, 失败返回-1
*/
uint32 sys_fstat(uint32 fd, file_stat_t *stat)
{
	return syscall6(SYS_FSTAT, (long)(fd), (long)(stat), 0, 0, 0, 0);
}

/*
	获取目录中的所有目录项信息
	成功返回读到的字节数, 失败返回-1
*/
uint32 sys_get_dentries(uint32 fd, dentry_t *buf, uint32 buf_len)
{
	return syscall6(SYS_GET_DENTRIES, (long)(fd), (long)(buf), (long)(buf_len), 0, 0, 0);
}

/*
	创建一个目录
	成功返回0, 失败返回-1
*/
uint32 sys_mkdir(char *path)
{
	return syscall6(SYS_MKDIR, (long)(path), 0, 0, 0, 0, 0);
}

/*
	修改当前工作目录
	成功返回0, 失败返回-1
*/
uint32 sys_chdir(char *new_path)
{
	return syscall6(SYS_CHDIR, (long)(new_path), 0, 0, 0, 0, 0);
}

/*
	打印当前工作目录的绝对路径
*/
uint32 sys_print_cwd()
{
	return syscall6(SYS_PRINT_CWD, 0, 0, 0, 0, 0, 0);
}

/*
	新建硬链接
	成功返回0, 失败返回-1
*/
uint32 sys_link(char *old_path, char *new_path)
{
	return syscall6(SYS_LINK, (long)(old_path), (long)(new_path), 0, 0, 0, 0);
}

/*
	解除硬链接
	成功返回0, 失败返回-1
*/
uint32 sys_unlink(char *path)
{
	return syscall6(SYS_UNLINK, (long)(path), 0, 0, 0, 0, 0);
}
#include <syscall_arch.h>
long syscall6(long n, long a, long b, long c, long d, long e, long f) { return arch_syscall6(n,a,b,c,d,e,f); }
