/* 16550 轮询输出；地址和寄存器间距由平台提供。 */
#include "uart16550.h"
static void write_reg(uint64 base, unsigned shift, unsigned reg, uint8 value)
{ *(volatile uint8 *)(base + ((uint64)reg << shift)) = value; }
void uart_init(uint64 base, unsigned clock, unsigned shift)
{
    unsigned divisor = (clock + 8 * 115200) / (16 * 115200);
    write_reg(base, shift, 1, 0);
    write_reg(base, shift, 3, 0x80);
    write_reg(base, shift, 0, divisor & 0xff);
    write_reg(base, shift, 1, divisor >> 8);
    write_reg(base, shift, 3, 3);
    write_reg(base, shift, 2, 7);
}
void uart_putc(uint64 base, unsigned shift, uint8 c)
{
    while ((*(volatile uint8 *)(base + (5UL << shift)) & 0x20) == 0) {}
    write_reg(base, shift, 0, c);
}
