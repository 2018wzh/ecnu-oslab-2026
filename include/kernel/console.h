#ifndef OSLAB_CONSOLE_H
#define OSLAB_CONSOLE_H
#include <kernel/types.h>
/* UART 初始化、同步字符输出和不依赖打印锁的紧急输出。 */
void console_init(void);
void console_putc(char c);
void emergency_puts(const char *s);
void console_input_init(void);
void console_input(uint8 c);
long console_read(void *dst, size_t len);
#endif
