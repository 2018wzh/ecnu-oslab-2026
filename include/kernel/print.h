#ifndef OSLAB_PRINT_H
#define OSLAB_PRINT_H
#include <kernel/types.h>
void print_init(void);
void print_unsigned(uint64 value, unsigned base);
void print_signed(int value);
void printf(const char *fmt, ...);
void assert(bool condition, const char *warning);
void panic(const char *message) __attribute__((noreturn));
#endif
