/* 外部中断控制器接口 (generic kernel): 接口在此声明, 实现由 drivers/irqchip 提供。
 * 两个平台都用 SiFive PLIC, 基地址来自 <platform.h> 的 PLAT_PLIC_BASE。 */
#ifndef __KERNEL_IRQ_H__
#define __KERNEL_IRQ_H__

#include <kernel/types.h>

// 全局初始化 (设置中断源优先级)。
void plic_init(void);

// 每个 hart 的初始化 (使能中断源 + 设置阈值); 每个 hart 都必须调用。
void plic_init_hart(void);

// 认领一个待处理中断; 返回中断号, 0 表示没有。
int plic_claim(void);

// 声明中断处理完成 (必须与 plic_claim 配对, 否则该中断源不再上报)。
void plic_complete(int irq);

#endif /* __KERNEL_IRQ_H__ */
