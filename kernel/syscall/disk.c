#include <kernel/syscall.h>
#include <kernel/fs.h>
#include <kernel/print.h>
/* 仅供受控例程：令牌不伪造、不重复归还、不携带未归还资源 fork/exit。
 * 参数读取、用户复制与调用顺序仍由学生完成；沿用前序简化失败契约。 */
/* TODO(lab-7): 申请 data block，返回绝对块号。 */
long sys_alloc_block(const syscall_args_t *call) { (void)call; panic("TODO(lab-7): sys_alloc_block"); }
/* TODO(lab-7): 归还绝对块号，成功返回 0。 */
long sys_free_block(const syscall_args_t *call) { (void)call; panic("TODO(lab-7): sys_free_block"); }
/* TODO(lab-7): 申请 inode，返回 inode 号，0 可分配。 */
long sys_alloc_inode(const syscall_args_t *call) { (void)call; panic("TODO(lab-7): sys_alloc_inode"); }
/* TODO(lab-7): 归还 inode 号，成功返回 0。 */
long sys_free_inode(const syscall_args_t *call) { (void)call; panic("TODO(lab-7): sys_free_inode"); }
/* TODO(lab-7): 选择 0=data、1=inode；成功 0，非法选择 -1。 */
long sys_show_bitmap(const syscall_args_t *call) { (void)call; panic("TODO(lab-7): sys_show_bitmap"); }
/* TODO(lab-7): 获取 buffer，成功返回内核地址令牌，失败 -1。 */
long sys_get_block(const syscall_args_t *call) { (void)call; panic("TODO(lab-7): sys_get_block"); }
/* TODO(lab-7): 将令牌对应 data 的完整 4096 字节复制到用户地址，成功 0。 */
long sys_read_block(const syscall_args_t *call) { (void)call; panic("TODO(lab-7): sys_read_block"); }
/* TODO(lab-7): 从用户地址复制完整 4096 字节到令牌对应 data，再写盘，成功 0。 */
long sys_write_block(const syscall_args_t *call) { (void)call; panic("TODO(lab-7): sys_write_block"); }
/* TODO(lab-7): 归还地址令牌且仅归还一次，成功 0。 */
long sys_put_block(const syscall_args_t *call) { (void)call; panic("TODO(lab-7): sys_put_block"); }
/* TODO(lab-7): 输出链表状态，成功 0。 */
long sys_show_buffer(const syscall_args_t *call) { (void)call; panic("TODO(lab-7): sys_show_buffer"); }
/* TODO(lab-7): 尝试释放 count 个非活跃数据页，成功返回 0（不是页数）。 */
long sys_flush_buffer(const syscall_args_t *call) { (void)call; panic("TODO(lab-7): sys_flush_buffer"); }
