#include <kernel/fs.h>
#include <kernel/print.h>
/* 教师诊断：fs_init 成功后调用；每次只持有一个位图 buffer。 */
void bitmap_print(bool print_data)
{
    bool inode = !print_data;
    uint32 first = inode ? superblock.inode_bitmap : superblock.data_bitmap;
    uint32 blocks = inode ? superblock.inode_bitmap_blocks : superblock.data_bitmap_blocks;
    uint32 total = inode ? superblock.total_inodes : superblock.data_blocks;
    uint32 base = inode ? 0 : superblock.data_first;
    if ((uint64)blocks * BLOCK_SIZE * 8 < total ||
        (uint64)first + blocks > superblock.total_blocks ||
        (!inode && (uint64)base + total > superblock.total_blocks)) panic("bitmap layout");
    printf("%s bitmap alloced bits:\n", inode ? "inode" : "data");
    for (uint64 start = 0; start < total; start += BLOCK_SIZE * 8) {
        buffer_t *b = buffer_get(first + start / (BLOCK_SIZE * 8));
        if (!b) panic("bitmap read");
        uint32 valid = total - start;
        if (valid > BLOCK_SIZE * 8) valid = BLOCK_SIZE * 8;
        for (uint32 bit = 0; bit < valid; ++bit)
            if (b->data[bit / 8] & (1U << (bit % 8)))
                printf("%d ", (uint32)(base + start + bit));
        buffer_put(b);
    }
    printf("over!\n\n");
}
// TODO(lab-7): 获取位图缓存，置位后写盘并归还；单个位图块内扫描 valid 个有效 bit，置首个零位，返回块内 bit 号；满返回 BLOCK_UNUSED。
uint32 bitmap_search_and_set(uint32 bitmap_block, uint32 valid) { (void)bitmap_block; (void)valid; panic("TODO(lab-7): bitmap_search_and_set"); }
// TODO(lab-7): 将单个位图块中 bit 清零。获取位图缓存、清位、写盘并归还。
void bitmap_clear(uint32 bitmap_block, uint32 bit) { (void)bitmap_block; (void)bit; panic("TODO(lab-7): bitmap_clear"); }
// TODO(lab-7): 跨位图块扫描、置位并写回，返回绝对块号；耗尽 panic。
uint32 bitmap_alloc_block(void) {  panic("TODO(lab-7): bitmap_alloc_block"); }
// TODO(lab-7): 跨位图块扫描、置位并写回，返回 inode 号（包括 0）；耗尽 panic。
uint32 bitmap_alloc_inode(void) {  panic("TODO(lab-7): bitmap_alloc_inode"); }
// TODO(lab-7): 将绝对块号转为位图位置，清位并写回。
void bitmap_free_block(uint32 number) { (void)number; panic("TODO(lab-7): bitmap_free_block"); }
// TODO(lab-7): 将 inode 号转为位图位置，清位并写回。
void bitmap_free_inode(uint32 number) { (void)number; panic("TODO(lab-7): bitmap_free_inode"); }
