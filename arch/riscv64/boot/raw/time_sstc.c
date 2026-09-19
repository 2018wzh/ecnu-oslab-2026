// 裸机直启的时钟 (Sstc 扩展)。
// 有固件时走 SBI TIME 扩展 (M-mode 写 mtimecmp); 无固件时 S-mode 写 stimecmp。
// M-mode 入口 (raw/entry_m.S) 需先开 menvcfg.STCE, 否则写 stimecmp 会非法指令。
// 不写 CLINT MMIO: 那是 M-mode 的 mtimecmp, S-mode 加了也看不到.
#include <kernel/types.h>
#include <kernel/arch.h>
#include <kernel/timer.h>

// Sstc 的"下一次中断时刻"寄存器 (CSR 0x14D)。
// 用编号而不用符号名: 老汇编器不认识 stimecmp 这个名字。
#define CSR_STIMECMP 0x14d

void timer_set_next(uint64 interval)
{
        // 必须是"现在 + 间隔"的绝对时刻, 理由与 SBI 版本相同。
        uint64 deadline = timer_read_cycles() + interval;

        asm volatile("csrw 0x14d, %0" ::"r"(deadline) : "memory");
}
