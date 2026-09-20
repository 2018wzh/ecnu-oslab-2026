#ifndef OSLAB_UAPI_H
#define OSLAB_UAPI_H
#define SYS_BRK 1
#define SYS_MMAP 2
#define SYS_MUNMAP 3
#define SYS_PRINT_STR 4
#define SYS_PRINT_INT 5
#define SYS_GETPID 6
#define SYS_FORK 7
#define SYS_WAIT 8
#define SYS_EXIT 9
#define SYS_SLEEP 10
#define E_BADARG (-1L)
#define SYS_ALLOC_BLOCK 11
#define SYS_FREE_BLOCK 12
#define SYS_ALLOC_INODE 13
#define SYS_FREE_INODE 14
#define SYS_SHOW_BITMAP 15
#define SYS_GET_BLOCK 16
#define SYS_READ_BLOCK 17
#define SYS_WRITE_BLOCK 18
#define SYS_PUT_BLOCK 19
#define SYS_SHOW_BUFFER 20
#define SYS_FLUSH_BUFFER 21
#endif
