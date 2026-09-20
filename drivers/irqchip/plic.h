#ifndef OSLAB_PLIC_H
#define OSLAB_PLIC_H
#include <kernel/types.h>
/* 所有 base/context 由平台提供；lab-2 已映射有效 MMIO。 */
void plic_init(uint64 base, unsigned irq);
void plic_enable(uint64 base, unsigned context, unsigned irq);
/* 0 表示没有待处理来源；非零来源处理结束后向同一 context complete。 */
unsigned plic_claim(uint64 base, unsigned context);
void plic_complete(uint64 base, unsigned context, unsigned irq);
#endif
