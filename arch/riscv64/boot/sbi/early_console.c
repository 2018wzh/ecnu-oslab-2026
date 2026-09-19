// 早期控制台 (走固件 SBI)。
// "最早的输出往哪写"取决于有没有固件: 有固件用 SBI 控制台, 无固件直接写 UART。
// 由 mk/boot/<boot>.mk 选择实现, arch.h 接口名保持不变。
// 除了启动调试, 它还能在切完 satp 后输出一行, 验证执行流越过分页这一步。
#include <kernel/types.h>
#include <kernel/arch.h>
#include <platform.h>
#include <asm/sbi.h>

void arch_early_puts(const char *s)
{
	while (*s)
		sbi_console_putchar(*s++);
}
