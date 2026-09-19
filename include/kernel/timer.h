/* 系统时钟接口 (generic kernel): 分 generic 滴答计数 (kernel/trap/timer.c)
 * 与平台硬件操作 (drivers/timer/) 两层, 对使用者是一个整体概念。 */
#ifndef __KERNEL_TIMER_H__
#define __KERNEL_TIMER_H__

#include <kernel/types.h>

// generic 层: 系统滴答 (kernel/trap/timer.c)。
// 初始化系统时钟状态。
void timer_create(void);

// 时钟中断到来时推进计数 (只由 cpuid 0 实际推进)。
void timer_update(void);

// 读取系统启动至今的滴答数。
uint64 timer_get_ticks(void);

// 睡眠指定的滴答数 (lab-6 完善)。
void timer_wait(uint64 ntick);

// 平台层: 硬件操作 (drivers/timer/, 由 mk/platform 选择实现)。
// 读取当前时间计数器 (单位 tick, 频率由平台决定)。
uint64 timer_read_cycles(void);

// 设置下一次时钟中断 (参数是绝对时间点, 不是间隔)。
void timer_set_next(uint64 interval);

// 该平台的时钟中断间隔 (tick 数); 不同平台数值不同但都约 0.1 秒一次。
uint64 timer_interval(void);

#endif /* __KERNEL_TIMER_H__ */
