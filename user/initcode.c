/* 第一个用户进程 (init), lab-4..lab-8 阶段版本。
 * 只证明"用户态 -> ecall -> trap -> syscall"链路是通的, 用 SYS_HELLOWORLD
 * (内核打印固定字符串) 输出, 不用 write/fd 表 —— 那属于 lab-9。
 * 本程序被内核以二进制形式嵌入 (见 mk/build.mk 的 initcode.h 规则),
 * 在系统启动时作为第一个用户进程加载执行。 */

#include <uapi/syscall.h>
#include "syscall_arch.h"

void main(void)
{
	__syscall0(SYS_HELLOWORLD);
	__syscall0(SYS_HELLOWORLD);

	/* 本阶段还没有进程生命周期管理, 打印完就地空转等待。 */
	for (;;)
		;
}