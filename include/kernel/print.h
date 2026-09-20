#ifndef OSLAB_PRINT_H
#define OSLAB_PRINT_H
#include <kernel/types.h>
/* 标准输出与错误处理；辅助函数不负责格式分派或互斥。 */
void print_init(void);
void print_unsigned(uint64 value, unsigned base);
void print_signed(int value);
void print_pointer(const void *pointer);
void printf(const char *fmt, ...);
void assert(bool condition, const char *warning);
void panic(const char *message) __attribute__((noreturn));
#endif
