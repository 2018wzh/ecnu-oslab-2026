#include <kernel/inode.h>
#include <kernel/fs.h>
#include <kernel/print.h>
superblock_t superblock;
/* 输出超级块与磁盘布局信息（教师诊断）；参数已经过 fs_init 校验。 */
void sb_print(const superblock_t *sb)
{
    printf("superblock: magic=0x%x block_size=%d\n", sb->magic, sb->block_size);
    printf("total_blocks= %d total_inodes= %d\n", sb->total_blocks, sb->total_inodes);
    printf("inode_bitmap= %d blocks= %d inode_first= %d blocks= %d\n",
           sb->inode_bitmap, sb->inode_bitmap_blocks, sb->inode_first, sb->inode_blocks);
    printf("data_bitmap= %d blocks= %d data_first= %d blocks= %d\n",
           sb->data_bitmap, sb->data_bitmap_blocks, sb->data_first, sb->data_blocks);
}
// TODO(lab-7): 在首进程上下文初始化 buffer 并读块 0；按 LE 解码、验证布局后打印。
// 不能在调度器启动前执行可能睡眠的磁盘 I/O。
void fs_init(void) {
    panic("TODO(lab-7): fs_init");
    /* TODO(lab-8): 完成前序初始化后，在合适位置调用 inode_init。 */
    /* TODO(lab-9): inode 初始化后调用 file_init 和 device_init。 */
}
