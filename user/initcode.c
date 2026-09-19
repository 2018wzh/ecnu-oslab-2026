/* 第一个用户进程 (init): 被内核以二进制嵌入并在启动时加载。
 * 先打印验证用户态->syscall 链路, 再 fork+exec 磁盘上的测试程序并 wait 其退出状态。
 * 它单独链接 (不带 help.c, 映像尽量小), 故自己写最小 puts_; 调用号来自 uapi/syscall.h。 */

#include <uapi/syscall.h>
#include "syscall_arch.h"

static void puts_(const char *s)
{
	long n = 0;
	while (s[n])
		n++;
	__syscall3(SYS_WRITE, STDOUT_FILENO, (long)s, n);
}

// 跑一个测试程序: fork -> 子进程 exec -> 父进程 wait; 返回子进程退出状态 (负数表示失败)。
static long run_test(const char *path)
{
	long pid = __syscall0(SYS_FORK);
	if (pid < 0)
		return -1;

	if (pid == 0) {
		// 子进程: exec 换成测试程序, 成功不返回。
		__syscall2(SYS_EXEC, (long)path, 0);
		// 只有 exec 失败才会到这里。
		puts_("init: exec 失败: ");
		puts_(path);
		puts_("\n");
		__syscall1(SYS_EXIT, 127);
	}

	// 父进程: 等子进程结束。
	int status = -1;
	long waited = __syscall1(SYS_WAIT, (long)&status);
	if (waited < 0)
		return -1;
	return status;
}

void main(void)
{
	static const char *tests[] = {
		"/test_1",
	};	const int ntest = (int)(sizeof(tests) / sizeof(tests[0]));

	puts_("========================================\n");
	puts_("  你好, 这里是用户态! (U-mode)\n");
	puts_("  ECNU OSLab 2026\n");
	puts_("========================================\n");
	puts_("这个字符串是通过系统调用 write() 输出的,\n");
	puts_("说明: 用户态 -> ecall -> trap -> syscall 分发的完整链路已经打通。\n");

	// 依次运行磁盘上的测试程序。
	puts_("\n======== 测试开始 ========\n");
	for (int i = 0; i < ntest; i++) {
		long st = run_test(tests[i]);
		puts_("-------- ");
		puts_(tests[i]);
		if (st == 0)
			puts_(": 通过 --------\n");
		else
			puts_(": 失败 --------\n");
	}
	puts_("======== 测试结束 ========\n");

	puts_("\ninit 进程退出。\n");
	__syscall1(SYS_EXIT, 0);

	// 不应到这里。
	for (;;)
		;
}
