#ifndef OSLAB_FS_H
#define OSLAB_FS_H
#include <kernel/lock.h>
#include <uapi/disk.h>
#define N_BUFFER_TEST 8
#ifndef N_BUFFER
#define N_BUFFER 16384
#endif
#define BLOCK_UNUSED 0xffffffffU
typedef struct buffer {
    uint32 block, refs;
    bool valid, disk;
    int io_result;
    uint8 *data;
    sleeplock_t lock;
    struct buffer *prev, *next;
} buffer_t;
typedef struct { uint32 magic, block_size, total_blocks, total_inodes;
    uint32 inode_bitmap, inode_bitmap_blocks, inode_first, inode_blocks;
    uint32 data_bitmap, data_bitmap_blocks, data_first, data_blocks; } superblock_t;
extern superblock_t superblock;
int block_init(void);
int block_rw(buffer_t *b, bool write);
void block_interrupt(void);
void block_map(void);
void buffer_init(void);
buffer_t *buffer_get(uint32 block);
void buffer_put(buffer_t *b);
void buffer_read(buffer_t *b);
void buffer_write(buffer_t *b);
uint32 buffer_freemem(uint32 count);
/* 教师辅助：持有 buffer_lock，哨兵已初始化，node 不是哨兵。 */
void buffer_move(buffer_t *node, bool active, bool front);
void buffer_print_info(void);
void fs_init(void);
void sb_print(const superblock_t *sb);
uint32 bitmap_search_and_set(uint32 bitmap_block, uint32 valid);
void bitmap_clear(uint32 bitmap_block, uint32 bit);
uint32 bitmap_alloc_block(void);
uint32 bitmap_alloc_inode(void);
void bitmap_free_block(uint32 number);
void bitmap_free_inode(uint32 number);
void bitmap_print(bool print_data);
#endif
