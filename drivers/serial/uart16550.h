#ifndef OSLAB_UART_H
#define OSLAB_UART_H
#include <kernel/types.h>
void uart_init(uint64 base, unsigned clock, unsigned shift);
void uart_putc(uint64 base, unsigned shift, uint8 c);
#endif
