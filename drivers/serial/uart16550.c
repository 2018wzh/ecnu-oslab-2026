/* 16550 轮询输出；地址和寄存器间距由平台提供。 */
#include "uart16550.h"
static void write_reg(uint64 base, unsigned shift, unsigned reg, uint8 value)
{ *(volatile uint8 *)(base + ((uint64)reg << shift)) = value; }
void uart_init(uint64 base, unsigned clock, unsigned shift)
{
    unsigned divisor = (clock + 8 * 115200) / (16 * 115200);
    /* 关闭 UART 中断；本章使用轮询输出。 */
    write_reg(base, shift, 1, 0);
    /* 打开波特率除数锁存，按平台时钟设置 115200 波特率的低位和高位。 */
    write_reg(base, shift, 3, 0x80);
    write_reg(base, shift, 0, divisor & 0xff);
    write_reg(base, shift, 1, divisor >> 8);
    /* 8 位数据、无校验；清空并使能 FIFO。 */
    write_reg(base, shift, 3, 3);
    write_reg(base, shift, 2, 7);
}
void uart_putc(uint64 base, unsigned shift, uint8 c)
{
    /* LSR 的发送空闲位表示 THR 可以接收下一个字符。 */
    while ((*(volatile uint8 *)(base + (5UL << shift)) & 0x20) == 0) {}
    write_reg(base, shift, 0, c);
}
int uart_getc(uint64 base, unsigned shift)
{
    if ((*(volatile uint8 *)(base + (5UL << shift)) & 1) == 0) return -1;
    return *(volatile uint8 *)base;
}
void uart_enable_rx(uint64 base, unsigned shift) { write_reg(base, shift, 1, 1); }
