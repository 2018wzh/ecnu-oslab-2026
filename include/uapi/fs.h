#ifndef OSLAB_UAPI_FS_H
#define OSLAB_UAPI_FS_H
#include <kernel/types.h>
typedef struct { uint16 type, nlink; uint32 size, inode_num, offset; } file_stat_t;
typedef struct { char name[60]; uint32 inode_num; } dirent_t;
_Static_assert(sizeof(file_stat_t) == 16, "stat ABI");
_Static_assert(offsetof(file_stat_t, offset) == 12, "stat offset ABI");
_Static_assert(sizeof(dirent_t) == 64, "dentry ABI");
#endif
