#include <kernel/proc.h>
#include <kernel/file.h>
#include <kernel/print.h>
// TODO(lab-9): 首进程 cwd=root，依次打开 stdin/stdout/stderr 为 fd0/1/2；失败回滚。
int proc_files_init(proc_t *p) { (void)p; panic("TODO(lab-9): proc_files_init"); }
// TODO(lab-9): fork 增加每个 file 和 cwd 的引用，文件偏移共享；不复用裸所有权。
int proc_files_clone(proc_t *parent, proc_t *child)
{ (void)parent; (void)child; panic("TODO(lab-9): proc_files_clone"); }
// 生命周期任务在 lifecycle.c；这里的拆分不增加学生任务。
/* 构建 fd -> file 的映射，返回 fd（教师辅助）。
 * 调用者独占 p 的文件表；成功转移一个引用，失败不消费引用。
 */
int fd_alloc(proc_t *p, file_t *f)
{
    if (!f) return -1;
    for (unsigned fd = 0; fd < N_FD; ++fd) {
        if (!p->files[fd]) { p->files[fd] = f; return (int)fd; }
    }
    return -1;
}
/* 返回 fd 对应的文件（教师范围与空槽检查）。
 * 参数解码留在 syscall 架构边界；这里不截断原始编号，不增加引用。
 */
file_t *fd_get(proc_t *p, uint64 fd)
{ return fd < N_FD ? p->files[fd] : NULL; }
