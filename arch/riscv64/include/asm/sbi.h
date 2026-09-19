#ifndef OSLAB_SBI_H
#define OSLAB_SBI_H
#include <kernel/types.h>
long sbi_call(uint64 extension, uint64 function, uint64 x, uint64 y, uint64 z);
void sbi_putchar(char c);
#endif
