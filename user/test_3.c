/* 测试三: fork / wait / exit 状态传递。
 * 验证 fork 后父子返回值不同、子进程退出状态能传回父进程、wait 真的在等待。 */
#include "help.h"

void main(void)
{
	uputs("---- test_3: fork / wait / exit ----\n");

	long pid = fork();
	if (pid < 0) {
		uprintf("[test_3] FAIL: fork 返回 %d\n", pid);
		exit(1);
	}

	if (pid == 0) {
		// 子进程: 打印后以状态 7 退出 (7 是"一眼能认出"的值)。
		uprintf("[test_3] 子进程 pid=%d, 即将以状态 7 退出\n", getpid());
		exit(7);
	}

	// 父进程: fork 返回的是子进程的 pid。
	uprintf("[test_3] 父进程 fork 返回子 pid=%d\n", pid);

	int status = 0;
	long waited = wait(&status);
	uprintf("[test_3] wait 返回 %d, 子进程状态 = %d (期望 7)\n",
		waited, status);

	if (status != 7)
		exit(1);

	uputs("[test_3] OK\n");
	exit(0);
}
