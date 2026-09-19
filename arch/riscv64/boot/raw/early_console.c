// 早期控制台 (直接写 UART 数据寄存器)。
// 裸机直启没有固件, 也没有 SBI 控制台, 只能直接写 UART。固件/QEMU 默认已把
// UART 留在可用状态, 波特率与中断配置在 console_init() 之后才做。
#include <kernel/types.h>
#include <kernel/arch.h>
#include <platform.h>

void arch_early_puts(const char *s)
{
        volatile uint8 *thr = (volatile uint8 *)PLAT_UART0_BASE;

        while (*s)
                *thr = (uint8)*s++;
}
