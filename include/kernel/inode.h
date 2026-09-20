#ifndef OSLAB_INODE_H
#define OSLAB_INODE_H
#include <kernel/fs.h>
#define N_INODE_CACHE 64
/* 磁盘索引节点的逻辑字段（64 字节）；按固定偏移 LE 编解码。 */
typedef struct inode_disk {
    uint16 type;                     // DATA 流式数据 / DIR 目录 / DEVICE 设备
    uint16 major, minor;             // 主/次设备号，默认 1/1
    uint16 nlink;                    // 磁盘链接数，不同于内存引用数
    uint32 size;                     // 数据长度；目录为有效槽字节总数
    uint32 index[INODE_INDEX_3];     // 10 直接 + 2 一级 + 1 二级
} inode_disk_t;
/* 内存资源；N_INODE_CACHE 是缓存数，N_INODE 是磁盘 inode 数。 */
typedef struct inode {
    inode_disk_t disk_info;          // 持久化信息，slk 保护
    bool valid_info;                 // 磁盘信息是否已读入，slk 保护
    uint32 inode_num;                // 磁盘身份，活跃引用期间不变
    uint32 ref;                      // 使用权计数，lk_inode_cache 保护
    sleeplock_t slk;                 // 磁盘 I/O 可睡眠，不能用自旋锁替代
} inode_t;
typedef struct dentry {
    char name[MAXLEN_FILENAME];     // 60 字节字段，首字节 0 表示空槽
    uint32 inode_num;               // 0 为合法根编号，失败用 INVALID_INODE_NUM
} dentry_t;
_Static_assert(sizeof(inode_disk_t) == 64, "inode logical size");
_Static_assert(sizeof(dentry_t) == 64, "dentry logical size");
void inode_init();
void inode_rw(inode_t *ip, bool write);
inode_t *inode_get(uint32 inode_num);
inode_t *inode_create(uint16 type, uint16 major, uint16 minor);
inode_t* inode_dup(inode_t* ip);
void inode_lock(inode_t* ip);
void inode_unlock(inode_t *ip);
void inode_put(inode_t* ip);
void inode_delete(inode_t *ip);
uint32 inode_read_data(inode_t *ip, uint32 offset, uint32 len, void *dst, bool is_user_dst);
uint32 inode_write_data(inode_t *ip, uint32 offset, uint32 len, void *src, bool is_user_src);
void inode_print(inode_t *ip, char* name);

/* dentry.c: 关于目录项和文件路径 */

uint32 dentry_search(inode_t *ip, char *name);
uint32 dentry_create(inode_t *ip, uint32 inode_num, char *name);
uint32 dentry_delete(inode_t *ip, char *name);
void dentry_print(inode_t *ip);
inode_t* path_to_inode(char *path);
inode_t* path_to_parent_inode(char *path, char *name);

void lab8_examples(void);
#endif
