// 时间的架构基元 (与启动协议无关)。
// timer_read_cycles() 用 rdtime CSR (S-mode 可读, 与有无固件无关) 属架构层;
// timer_interval() 来自 platform 层常量, 但"怎么取"是架构层的事。
// timer_set_next() 不在此处: 它取决于固件 (SBI TIME / CPU Sstc / M-mode 转发),
// 由启动路径提供 (见 boot/{sbi,raw}/time*.c)。读时间与设时间依赖不同, 拆开。
#include <kernel/types.h>
#include <kernel/arch.h>
#include <platform.h>

// 读取当前时间 (tick)。rdtime 在 QEMU 与真机上都映射到 mtime, 不需读 MMIO。
uint64 timer_read_cycles(void)
{
        return arch_read_time();
}

// 时钟中断间隔 (tick 数)。两平台数值不同 (CPU 频率) 但"每 0.1 秒一次"语义相同,
// 数值差异由 platform 层吸收。
uint64 timer_interval(void)
{
        return PLAT_TIMER_INTERVAL;
}
