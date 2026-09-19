// 系统时钟状态 (generic kernel)。
// 这里没有 MMIO 地址/CLINT/satp, 只维护"系统运行了多少 tick"的逻辑状态;
// tick 从哪来 (读 CLINT 还是 SBI) 是 drivers/timer/ 的事。
#include <kernel/types.h>
#include <kernel/sync.h>
#include <kernel/arch.h>

typedef struct timer {
	uint64 ticks;      /* 系统启动至今的滴答数 */
	spinlock_t lk;
} timer_t;

static timer_t sys_timer;

void timer_create(void)
{
}
void timer_update(void)
{
}

uint64 timer_get_ticks(void)
{
	return sys_timer.ticks;
}
