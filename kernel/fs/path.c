#include <kernel/proc.h>
#include <kernel/file.h>
#include <kernel/print.h>
// TODO(lab-9): 按 inode 锁保护目录修改；新目录建立 . 和 ..；失败回滚。
inode_t *path_create(const char *path, uint16 type, uint16 major, uint16 minor)
{ (void)path; (void)type; (void)major; (void)minor; panic("TODO(lab-9): path_create"); }
// TODO(lab-9): 普通文件硬链接，先增 nlink 再加目录项，失败回滚；拒绝目录硬链接。
int path_link(const char *oldpath, const char *newpath)
{ (void)oldpath; (void)newpath; panic("TODO(lab-9): path_link"); }
// TODO(lab-9): 删除目录项并减 nlink，inode_put 在最后引用时判断资源回收。
int path_unlink(const char *path) { (void)path; panic("TODO(lab-9): path_unlink"); }
// TODO(lab-9): 经 .. 回溯，用 dentry_search_number 在父目录反查名字；根为 /；逆向填充，返回起始字节偏移（dst + offset），容量不足 -1。
int inode_to_path(inode_t *ip, char *dst, size_t capacity)
{ (void)ip; (void)dst; (void)capacity; panic("TODO(lab-9): inode_to_path"); }
