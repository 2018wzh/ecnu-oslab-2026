/* 测试一: 基本输出与系统调用返回值。
 * exec 暂不支持传参, 故保留"输出 + 返回值检查"主线: 证明用户态 write
 * 到达内核且返回值正确。 */
#include "help.h"

void main(void)
{
	uputs("---- test_1: 基本输出 ----\n");

	// stdout 与 stderr 走同一控制台但 fd 不同, 内核要分别处理; 长度用 sizeof-1 而非手写。
	static const char to_out[] = "[test_1] 这一行走 stdout\n";
	static const char to_err[] = "[test_1] 这一行走 stderr\n";
	write(STDOUT_FILENO, to_out, sizeof(to_out) - 1);
	write(STDERR_FILENO, to_err, sizeof(to_err) - 1);

	// 返回值检查: write 返回实际写出的字节数, 负数表示失败。
	const char msg[] = "abcdef";
	long n = write(STDOUT_FILENO, msg, 6);
	if (n != 6) {
		uprintf("[test_1] FAIL: write 返回 %d, 期望 6\n", n);
		exit(1);
	}
	uputc('\n');

	uprintf("[test_1] write 返回 %d (正确)\n", n);
	uprintf("[test_1] getpid() = %d\n", getpid());
	uputs("[test_1] OK\n");
	exit(0);
}
