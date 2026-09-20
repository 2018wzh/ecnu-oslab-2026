#ifndef OSLAB_CONSOLE_H
#define OSLAB_CONSOLE_H
/* UART 初始化、同步字符输出和不依赖打印锁的紧急输出。 */
void console_init(void);
void console_putc(char c);
void emergency_puts(const char *s);
#endif
