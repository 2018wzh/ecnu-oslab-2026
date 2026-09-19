/* 测试四: exec 替换自身。
 * 验证 exec 成功后不返回 (进程映像被整个换掉), 若 exec 之后的打印出现则说明不是真 exec。 */
#include "help.h"

void main(void)
{
	uputs("---- test_4: exec 替换自身 ----\n");
	uprintf("[test_4] 我是 pid=%d, 现在 exec /test_1\n", getpid());

	long r = exec("/test_1");

	// exec 成功的话永远到不了这里。
	uprintf("[test_4] FAIL: exec 返回了 %d\n", r);
	exit(1);
}
