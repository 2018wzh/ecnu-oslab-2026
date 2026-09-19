/* 测试二: 普通文件操作 (open/close/read/write/lseek)。
 * 走"创建 -> 写 -> 定位 -> 读回 -> 关闭"的通路, 覆盖块设备驱动、缓冲缓存、inode 与文件表四层。 */
#include "help.h"

void main(void)
{
	uputs("---- test_2: 文件读写 ----\n");

	int fd = open("/TEST.TXT", O_RDWR | O_CREATE);
	if (fd < 0) {
		uprintf("[test_2] FAIL: 创建 /TEST.TXT 失败 (%d)\n", fd);
		exit(1);
	}

	const char text[] = "hello, oslab 2026";
	long n = write(fd, text, sizeof(text) - 1);
	uprintf("[test_2] 写入 %d 字节\n", n);

	// 回到文件开头再读: 不 lseek 的话 off 停在末尾, read 返回 0 (读到 0 字节而非错误)。
	long off = lseek(fd, 0, 0 /* SEEK_SET */);
	uprintf("[test_2] lseek 回到 %d\n", off);

	char buf[32];
	n = read(fd, buf, sizeof(text) - 1);
	if (n != (long)(sizeof(text) - 1)) {
		uprintf("[test_2] FAIL: read 返回 %d, 期望 %d\n",
			n, (int)(sizeof(text) - 1));
		exit(1);
	}
	buf[n] = '\0';
	uprintf("[test_2] 读回: %s\n", buf);

	if (close(fd) < 0) {
		uputs("[test_2] FAIL: close 失败\n");
		exit(1);
	}

	uputs("[test_2] OK\n");
	exit(0);
}
