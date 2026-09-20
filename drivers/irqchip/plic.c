#include "plic.h"
#include <kernel/print.h>
/* 教师驱动：平台提供 MMIO 基址和当前 hart 的 S-mode context。 */
void plic_init(uint64 base, unsigned irq)
{
    assert(irq > 0 && irq < 1024, "plic_init: irq");
    *(volatile uint32 *)(base + 4UL * irq) = 1;
}
/* 每核初始化自己的 context；同一使能字的修改不得并发执行。 */
void plic_enable(uint64 base, unsigned context, unsigned irq)
{
    assert(irq > 0 && irq < 1024, "plic_enable: irq");
    assert(context < (0x04000000 - 0x200000) / 0x1000, "plic_enable: context");
    volatile uint32 *enable = (volatile uint32 *)(base + 0x2000 + 0x80UL * context + 4UL * (irq / 32));
    *enable = *enable | (1U << (irq % 32));
    *(volatile uint32 *)(base + 0x200000 + 0x1000UL * context) = 0;
}
unsigned plic_claim(uint64 base, unsigned context)
{
    assert(context < (0x04000000 - 0x200000) / 0x1000, "plic_claim: context");
    return *(volatile uint32 *)(base + 0x200004 + 0x1000UL * context);
}
void plic_complete(uint64 base, unsigned context, unsigned irq)
{ if (irq) *(volatile uint32 *)(base + 0x200004 + 0x1000UL * context) = irq; }
