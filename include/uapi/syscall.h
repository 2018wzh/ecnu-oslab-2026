#ifndef OSLAB_UAPI_H
#define OSLAB_UAPI_H
#define SYS_BRK 1
#define SYS_MMAP 2
#define SYS_MUNMAP 3
#define SYS_FORK 4
#define SYS_WAIT 5
#define SYS_EXIT 6
#define SYS_SLEEP 7
#define SYS_GETPID 8
#define SYS_EXEC 9
#define SYS_OPEN 10
#define SYS_CLOSE 11
#define SYS_READ 12
#define SYS_WRITE 13
#define SYS_LSEEK 14
#define SYS_DUP 15
#define SYS_FSTAT 16
#define SYS_GET_DENTRIES 17
#define SYS_MKDIR 18
#define SYS_CHDIR 19
#define SYS_PRINT_CWD 20
#define SYS_LINK 21
#define SYS_UNLINK 22
#define STR_MAXLEN 127
#define PATH_BYTES 128
#define MAX_ARGS 32
#define ARG_BYTES 128
#define OPEN_CREATE 1
#define OPEN_READ 2
#define OPEN_WRITE 4
#define LSEEK_SET 0
#define LSEEK_ADD 1
#define LSEEK_SUB 2
#define SYS_MAX_NUM 22
#define E_BADARG (-1L)
#endif
