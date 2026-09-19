/* 陷入现场 (trapframe) 的通用视图 (arch-api): 类型由架构定义,
 * generic kernel 通过下面按语义命名的宏存取, 不直接读写字段。 */
#ifndef __KERNEL_ARCH_TRAP_H__
#define __KERNEL_ARCH_TRAP_H__

#include <asm/trapframe.h>

// 陷入现场的类型, generic kernel 只当它是定长的、由 arch 解释的内存。
typedef struct trapframe trapframe_t;

// 按语义命名的存取宏 (实现见 arch/<arch>/include/asm/trapframe.h)。

// 出错/继续执行的指令地址。
#define TF_PC(tf)             ARCH_TF_PC(tf)
#define TF_SET_PC(tf, v)      ARCH_TF_SET_PC(tf, v)

// 用户栈指针。
#define TF_USER_SP(tf)        ARCH_TF_USER_SP(tf)
#define TF_SET_USER_SP(tf, v) ARCH_TF_SET_USER_SP(tf, v)

// 返回值寄存器: 系统调用返回值写这里; fork 靠把子进程此项改成 0 实现"一次调用、两次返回"。
#define TF_RET(tf)            ARCH_TF_RET(tf)
#define TF_SET_RET(tf, v)     ARCH_TF_SET_RET(tf, v)

// 把整个现场清零。
#define TF_ZERO(tf)           ARCH_TF_ZERO(tf)

#endif /* __KERNEL_ARCH_TRAP_H__ */
