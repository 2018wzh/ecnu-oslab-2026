/* 系统调用号 (用户态接口, UAPI): 被内核与用户程序共同包含,
 * 构成用户态可见契约 (调用号/参数约定/错误码), 不得含内核私有结构。
 * 用户程序 syscall 封装包含本文件, 保证调用号单一事实来源。 */
#ifndef __UAPI_SYSCALL_H__
#define __UAPI_SYSCALL_H__

/* 系统调用号: 从 1 开始 (0 保留为"非法") */
#define SYS_HELLOWORLD  0   /* 内核打印固定字符串 (lab-4 对齐 2025) */

#define SYS_EXIT    1   /* 退出当前进程 */
#define SYS_FORK    2   /* 创建子进程 */
#define SYS_READ    3   /* 读文件/设备 */
#define SYS_WRITE   4   /* 写文件/设备 */
#define SYS_EXEC    5   /* 执行可执行文件 */
#define SYS_WAIT    6   /* 等待子进程结束 */
#define SYS_BRK     8   /* 调整堆边界 (lab-5) */
#define SYS_MMAP    9   /* 内存映射 (lab-5) */
#define SYS_MUNMAP  14  /* 解除内存映射 (lab-5) */
#define SYS_GETPID  10  /* 取当前进程号 (调试用) */
#define SYS_OPEN    11  /* lab-9: 打开文件 */
#define SYS_CLOSE   12  /* lab-9: 关闭文件 */
#define SYS_LSEEK   13  /* lab-9: 移动读写位置 */

#define SYS_MAX     15

/* 系统调用号 (用户态接口, UAPI): 被内核与用户程序共同包含,
 * 构成用户态可见契约 (调用号/参数约定/错误码), 不得含内核私有结构。
 * 用户程序 syscall 封装包含本文件, 保证调用号单一事实来源。 */
#define STDIN_FILENO  0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

/* 错误码 (负数返回) */
#define E_OK        0
#define E_BADARG   -1
#define E_NOENT    -2
#define E_NOMEM    -3
#define E_IO       -4
#define E_NOFD     -5    /* 文件描述符无效或已用尽 */
#define E_BADFMT   -6    /* 可执行文件格式错误 */
#define E_NOSYS   -38    /* 该功能尚未实现 (与 Linux 的 ENOSYS 同值) */
#define O_RDONLY  0
#define O_WRONLY  1
#define O_RDWR    2
#define O_CREATE  4

#endif /* __UAPI_SYSCALL_H__ */
