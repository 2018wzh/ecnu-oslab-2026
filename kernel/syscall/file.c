#include <kernel/syscall.h>
#include <kernel/print.h>
// TODO(lab-9): path, argv：最多 32 个参数，每个含 NUL 最多 128 字节；构造后提交，成功 argc，失败 -1。
long sys_exec(const syscall_args_t *call) { (void)call; panic("TODO(lab-9): sys_exec"); }
// TODO(lab-9): path, mode：CREATE=1/READ=2/WRITE=4；路径含 NUL 最多 128 字节；成功 fd，失败 -1。
long sys_open(const syscall_args_t *call) { (void)call; panic("TODO(lab-9): sys_open"); }
// TODO(lab-9): fd：成功 0，失败 -1。
long sys_close(const syscall_args_t *call) { (void)call; panic("TODO(lab-9): sys_close"); }
// TODO(lab-9): fd, len, addr：成功字节数，失败 0；用户地址须经页表复制。
long sys_read(const syscall_args_t *call) { (void)call; panic("TODO(lab-9): sys_read"); }
// TODO(lab-9): fd, len, addr：成功字节数，失败 0；用户地址须经页表复制。
long sys_write(const syscall_args_t *call) { (void)call; panic("TODO(lab-9): sys_write"); }
// TODO(lab-9): fd, unsigned offset, SET/ADD/SUB：尽力而为移动偏移，成功返回新偏移，失败 -1。
long sys_lseek(const syscall_args_t *call) { (void)call; panic("TODO(lab-9): sys_lseek"); }
// TODO(lab-9): fd：增加共享 file 引用，成功新 fd，失败 -1。
long sys_dup(const syscall_args_t *call) { (void)call; panic("TODO(lab-9): sys_dup"); }
// TODO(lab-9): fd, addr：复制 type:u16,nlink:u16,size:u32,inode_num:u32,offset:u32；成功 0，失败 -1。
long sys_fstat(const syscall_args_t *call) { (void)call; panic("TODO(lab-9): sys_fstat"); }
// TODO(lab-9): fd, addr, buffer_len：容量与返回值均为字节；传输有效项，失败 -1。
long sys_get_dentries(const syscall_args_t *call) { (void)call; panic("TODO(lab-9): sys_get_dentries"); }
// TODO(lab-9): path：成功 0，失败 -1。
long sys_mkdir(const syscall_args_t *call) { (void)call; panic("TODO(lab-9): sys_mkdir"); }
// TODO(lab-9): path：替换 cwd 引用；成功 0，失败 -1。
long sys_chdir(const syscall_args_t *call) { (void)call; panic("TODO(lab-9): sys_chdir"); }
// TODO(lab-9): 无参数：逆向构造路径后从返回偏移打印；成功 0，失败 -1。
long sys_print_cwd(const syscall_args_t *call) { (void)call; panic("TODO(lab-9): sys_print_cwd"); }
// TODO(lab-9): old_path,new_path：成功 0，失败 -1。
long sys_link(const syscall_args_t *call) { (void)call; panic("TODO(lab-9): sys_link"); }
// TODO(lab-9): path：成功 0，失败 -1。
long sys_unlink(const syscall_args_t *call) { (void)call; panic("TODO(lab-9): sys_unlink"); }

// sys_exec 参数暂存总量可达 4096 字节；在独立页面中保存并统一释放，不能把整组参数放进一页内核栈。
