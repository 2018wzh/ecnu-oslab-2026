#ifndef OSLAB_PLATFORM_H
#define OSLAB_PLATFORM_H
#define PLATFORM_NAME "qemu-virt"
#define HART_FIRST 0
#define NCPU 2
#define DRAM_BASE 0x80000000UL
#define DRAM_SIZE (128UL * 1024 * 1024)
#define UART_BASE 0x10000000UL
#define UART_CLOCK 3686400
#define UART_SHIFT 0
/* 本章只映射 PLIC；中断控制留待下一章。 */
#define PLIC_BASE 0x0c000000UL
#define PLIC_SIZE 0x04000000UL
#endif
