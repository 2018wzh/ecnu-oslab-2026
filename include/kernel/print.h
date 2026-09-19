/* 内核格式化输出: 分层为 printf 格式化 -> console 字符抽象 -> UART 驱动 -> 平台地址,
 * 改串口地址只影响最底层。 */
#ifndef __KERNEL_PRINT_H__
#define __KERNEL_PRINT_H__

#include <kernel/types.h>

// 初始化打印子系统 (建立打印锁)。
void print_init(void);

// 格式化输出到控制台。
void printf(const char *fmt, ...);

// 输出到 panic 缓冲区 (用于崩溃时保证输出完整)。
void panic(const char *fmt, ...) __attribute__((noreturn));

// 底层字符输出, 由 console 实现。
void console_putc(int c);
int  console_getc(void);
void console_init(void);

// 原始字符输出 (不经过缓冲区, 崩溃时也能用)。
void console_putc_raw(int c);

#endif /* __KERNEL_PRINT_H__ */
