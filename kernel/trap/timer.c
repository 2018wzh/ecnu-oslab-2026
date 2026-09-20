#include <kernel/trap.h>
#include <kernel/arch.h>
#include <kernel/print.h>
#include <asm/csr.h>
#include <asm/sbi.h>
#include <platform.h>
/* 时钟创建：初始化各核心共享的系统时钟，仅在主核执行一次。 */
// TODO(lab-3): 初始化共享 ticks 及保护它的同步状态。
void timer_create(void) { panic("TODO(lab-3): timer_create"); }
/* 教师外围：读取 time，经 SBI TIME 设置本核的绝对截止时间。 */
void timer_init(void)
{
    uint64 now;
    __asm__ volatile("rdtime %0" : "=r"(now));
    if (sbi_call(0x54494d45, 0, now + TIMER_INTERVAL, 0, 0) != 0)
        panic("SBI timer init failed");
}
/* 全局系统时钟的更新；仅启动核调用。 */
// TODO(lab-3): 同步增加共享 ticks；不在此续订各核的硬件定时器。
void timer_update(void) { panic("TODO(lab-3): timer_update"); }
/* 教师中断外围：每核续订，启动核记账。
 * 固件启动核不一定是 hart 0；SBI TIME 续订同时撤销当前定时器的 pending 状态。
 */
void timer_tick(void)
{
    timer_init();
    if (arch_is_boot_cpu()) timer_update();
}
// TODO(lab-3): 同步读取共享计数，不能无锁读写普通变量。
uint64 timer_ticks(void) { panic("TODO(lab-3): timer_ticks"); }
