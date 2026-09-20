#include <kernel/proc.h>
#include <kernel/file.h>
#include <kernel/print.h>
file_t file_table[N_FILE];
spinlock_t file_table_lock;
// TODO(lab-9): 初始化文件池和表锁；字段恢复 inode/readable/writable/offset/refs。
void file_init(void) { panic("TODO(lab-9): file_init"); }
// TODO(lab-9): 表锁内取得 refs=0 的槽，初始化后返回。
file_t *file_alloc(void) { panic("TODO(lab-9): file_alloc"); }
// TODO(lab-9): 解析路径/按 OPEN_CREATE 创建，检查 OPEN_READ/OPEN_WRITE 与类型。
file_t *file_open(const char *path, uint32 mode)
{ (void)path; (void)mode; panic("TODO(lab-9): file_open"); }
// TODO(lab-9): 增引用，共享偏移，不复制 file。
file_t *file_dup(file_t *f) { (void)f; panic("TODO(lab-9): file_dup"); }
// TODO(lab-9): 最后引用时释放 inode；不能持 file_table 自旋锁做 I/O。
void file_close(file_t *f) { (void)f; panic("TODO(lab-9): file_close"); }
// TODO(lab-9): 按类型读取内核/用户目标；用户地址沿用 lab-8 页表复制；普通文件持 inode 锁完成共享 offset 的读改写。
long file_read(file_t *f, uint32 len, uint64 dst, bool is_user_dst)
{ (void)f; (void)dst; (void)len; (void)is_user_dst; panic("TODO(lab-9): file_read"); }
// TODO(lab-9): 按类型写内核缓冲；拒绝写目录，持 inode 锁更新实际完成量和共享 offset。
long file_write(file_t *f, uint32 len, uint64 src, bool is_user_src)
{ (void)f; (void)src; (void)len; (void)is_user_src; panic("TODO(lab-9): file_write"); }
// TODO(lab-9): unsigned offset；SET/ADD/SUB 尽力而为；普通文件 inode 锁保护共享偏移，成功返回新偏移、失败 -1。
long file_seek(file_t *f, uint32 offset, uint32 whence)
{ (void)f; (void)offset; (void)whence; panic("TODO(lab-9): file_seek"); }
// TODO(lab-9): inode 锁下形成固定布局 stat，不能泄露内核 padding。
int file_stat(file_t *f, file_stat_t *out)
{ (void)f; (void)out; panic("TODO(lab-9): file_stat"); }
