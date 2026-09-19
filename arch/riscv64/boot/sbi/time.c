// RISC-V 定时器 (rdtime 读 + SBI 设置), 一份服务 QEMU virt 与 VisionFive2。
// 读时间用 `rdtime` (S-mode 可读, 快); 设 mtimecmp 必须走 SBI TIME 扩展
// (mtimecmp 是 M-mode 寄存器, S-mode 直写会非法指令)。CLINT MMIO 因此不需要。
#include <kernel/types.h>
#include <kernel/arch.h>
#include <platform.h>
#include <asm/csr.h>
#include <kernel/timer.h>
#include <asm/sbi.h>

// 设置下一次时钟中断。参数是"从现在起多少个 tick", 这里换算成绝对时间点
// 再交给固件 (误传相对量会导致中断不再触发或疯狂触发)。
void timer_set_next(uint64 interval)
{
        sbi_set_timer(timer_read_cycles() + interval);
}
