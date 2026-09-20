#include <kernel/console.h>
#include <drivers/serial/uart16550.h>
#include <asm/sbi.h>
#include <platform.h>
void console_init(void) { uart_init(UART_BASE, UART_CLOCK, UART_SHIFT); }
void console_putc(char c)
{
    if (c == '\n') uart_putc(UART_BASE, UART_SHIFT, '\r');
    uart_putc(UART_BASE, UART_SHIFT, (uint8)c);
}
void emergency_puts(const char *s)
{
    while (*s) { if (*s == '\n') sbi_putchar('\r'); sbi_putchar(*s++); }
}
