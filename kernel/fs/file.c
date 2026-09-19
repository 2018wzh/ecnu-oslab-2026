// 文件对象与文件描述符表。
// 三层结构: fd (每个进程一份的私有索引) -> file 对象 (一次 open 产生,
// 存偏移量/权限/引用计数, fork 时共享) -> inode (文件本身, 所有打开者共享)。
// 中间的 file 层承载"一次打开"的状态: dup 时两个 fd 共享偏移量, 各自 open
// 同一文件时独立性偏移量, 只有 file 层能区分这两种情况。
#include <kernel/types.h>
#include <kernel/fs.h>
#include <uapi/syscall.h>

/* 实验阶段开关 (由构建系统定义; 未定义时等于 9 = 完整实现) */
#include <kernel/print.h>
#include <kernel/string.h>
#include <kernel/sync.h>
#include <kernel/proc.h>

static struct {
	spinlock_t lk;
	file_t file[NFILE];
} ftable;

void file_init(void)
{
	spinlock_init(&ftable.lk, "ftable");
	for (int i = 0; i < NFILE; i++)
		ftable.file[i].type = FD_NONE;
	printf("[fs] 系统打开文件表已初始化 (%d 项)\n", NFILE);
}
file_t *file_alloc(void)
{
	spinlock_acquire(&ftable.lk);

	for (int i = 0; i < NFILE; i++) {
		file_t *f = &ftable.file[i];
		if (f->ref == 0) {
			f->ref = 1;
			f->type = FD_NONE;      /* 先标记为无效 */
			f->off = 0;
			f->readable = 0;
			f->writable = 0;
			f->ip = NULL;
			spinlock_release(&ftable.lk);
			return f;
		}
	}

	spinlock_release(&ftable.lk);
	return NULL;
}

file_t *file_dup(file_t *f)
{
	spinlock_acquire(&ftable.lk);
	if (f->ref < 1)
		panic("file_dup: file 引用计数为 0");
	f->ref++;
	spinlock_release(&ftable.lk);
	return f;
}

void file_close(file_t *f)
{
	spinlock_acquire(&ftable.lk);

	if (--f->ref > 0) {
		spinlock_release(&ftable.lk);
		return;
	}

	/* 引用计数归零: 释放这个 file 对象。
	 * 注意此时才 inode_put —— 因为 file 一直持有 inode 的引用,
	 * 所以文件在打开期间不会被回收 (即使已经被 unlink)。
	 * 这就是"删除正在使用的文件"能正确工作的原因。 */
	file_t *ip_file = f;
	ip_file->type = FD_NONE;
	spinlock_release(&ftable.lk);

	/* 释放 inode 引用。这一步依赖 inode 层 (lab-8 引入)。
	 * lab-4..lab-7 阶段还没有 inode, file 对象也不会持有它。 */
	if (ip_file->ip)
		inode_put(ip_file->ip);
}
int fd_setup_stdio(struct proc *proc)
{
	proc_t *p = (proc_t *)proc;
	if (!p)
		return -1;
	file_t *cf = file_alloc();
	if (!cf)
		return -1;

	cf->type     = FD_CONSOLE;
	cf->readable = 1;
	cf->writable = 1;
	cf->off      = 0;
	cf->ip       = NULL;
	cf->ref      = 3;   /* 0/1/2 三处引用 */

	p->ofile[STDIN_FILENO]  = cf;
	p->ofile[STDOUT_FILENO] = cf;
	p->ofile[STDERR_FILENO] = cf;

	return 0;
}

/* 在进程的 fd 表中找一个空位, 返回 fd 号; 失败返回 -1 */

int fd_alloc(proc_t *p, file_t *f)
{
}

file_t *fd_get(proc_t *p, int fd)
{
	if (!p || fd < 0 || fd >= NOFILE)
		return NULL;
	return p->ofile[fd];
}

void fd_close(proc_t *p, int fd)
{
	if (!p || fd < 0 || fd >= NOFILE)
		return;

	spinlock_acquire(&p->lk);
	file_t *f = p->ofile[fd];
	p->ofile[fd] = NULL;
	spinlock_release(&p->lk);

	if (f)
		file_close(f);
}
