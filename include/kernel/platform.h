/* 平台抽象接口 (generic kernel 视角): 与 platform/<板子>/platform.h (机器描述宏) 不同,
 * 本文件是平台能力接口, generic kernel 通过它请平台层映射 MMIO、分发中断。 */
#ifndef __KERNEL_PLATFORM_H__
#define __KERNEL_PLATFORM_H__

#include <kernel/types.h>

/* --------------------------------------------------------------------------
 * 板级初始化 (打印板级信息等)
 * -------------------------------------------------------------------------- */
void platform_init(void);


// 次核的启动入口地址 (供 smp 层通过 SBI HSM 启动其他 hart)。
int platform_map_devices(uint64 pgtbl);


// 分发一个外部中断: 中断号->设备的对应关系机器相关, 由平台层决定。
// 返回 1 表示已处理, 0 表示不认识这个中断号。
#endif /* __KERNEL_PLATFORM_H__ */
