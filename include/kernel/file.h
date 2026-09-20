#ifndef OSLAB_FILE_H
#define OSLAB_FILE_H
#include <kernel/inode.h>
#include <uapi/syscall.h>
#include <uapi/fs.h>
#define N_FILE 128
#define N_FD 10
struct proc;
typedef struct file { inode_t *inode; bool readable, writable; uint32 offset, refs; } file_t;
// 用户 stat 固定 16 字节：u16 type,nlink；u32 size,inode_num,offset。
void file_init(void);
file_t *file_alloc(void);
file_t *file_open(const char *path, uint32 mode);
file_t *file_dup(file_t *f);
void file_close(file_t *f);
long file_read(file_t *f, uint32 len, uint64 dst, bool is_user_dst);
long file_write(file_t *f, uint32 len, uint64 src, bool is_user_src);
long file_seek(file_t *f, uint32 offset, uint32 whence);
int file_stat(file_t *f, file_stat_t *out);
int device_init(void);
long device_stdin(void *dst, size_t len);
long device_stdout(const void *src, size_t len);
long device_stderr(const void *src, size_t len);
long device_zero(void *dst, size_t len);
long device_null_read(void *dst, size_t len);
long device_null_write(const void *src, size_t len);
long device_gpt(const void *src, size_t len);
bool device_open_check(uint16 major, uint32 mode);
long device_read(uint16 major, void *dst, size_t len);
long device_write(uint16 major, const void *src, size_t len);
inode_t *path_create(const char *path, uint16 type, uint16 major, uint16 minor);
int path_link(const char *oldpath, const char *newpath);
int path_unlink(const char *path);
int inode_to_path(inode_t *ip, char *dst, size_t capacity);
int proc_files_init(struct proc *p);
int proc_files_clone(struct proc *parent, struct proc *child);
int fd_alloc(struct proc *p, file_t *f);
file_t *fd_get(struct proc *p, uint64 fd); /* 借用；调用期间文件表及对应引用保持有效。 */
#endif
