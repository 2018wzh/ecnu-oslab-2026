/* 用户态辅助函数 (不依赖任何库): 用户程序没有 libc,
 * 这里提供架构无关的字符串/输出格式化/读一行, 系统调用在 user/syscall.c。 */

#include "help.h"

unsigned long ustrlen(const char *s)
{
	unsigned long n = 0;
	while (s[n])
		n++;
	return n;
}

void uputs(const char *s)
{
	write(STDOUT_FILENO, s, ustrlen(s));
}

void uputc(char c)
{
	write(STDOUT_FILENO, &c, 1);
}

// 输出一个整数 (十进制)。
void uputint(long v)
{
	char buf[24];
	int i = 0;
	int neg = 0;
	unsigned long u;

	if (v < 0) {
		neg = 1;
		u = (unsigned long)(-v);
	} else {
		u = (unsigned long)v;
	}

	if (u == 0) {
		buf[i++] = '0';
	} else {
		while (u) {
			buf[i++] = (char)('0' + (u % 10));
			u /= 10;
		}
	}

	if (neg)
		uputs("-");

	// 数字是逆序生成的, 需要反过来输出。
	while (i > 0)
		write(STDOUT_FILENO, &buf[--i], 1);
}

// 输出一个整数 (十六进制, 小写, 无前缀)。
void uputhex(unsigned long v)
{
	static const char digits[] = "0123456789abcdef";
	char buf[16];
	int start = 0;

	for (int i = 0; i < 16; i++)
		buf[i] = digits[(v >> ((15 - i) * 4)) & 0xf];

	// 去掉前导零但至少留一位。
	while (start < 15 && buf[start] == '0')
		start++;
	write(STDOUT_FILENO, buf + start, (unsigned long)(16 - start));
}

// 极简 printf: 支持 %d %s %x %c %%, 保持简单便于扩展。
void uprintf(const char *fmt, ...)
{
	__builtin_va_list ap;
	__builtin_va_start(ap, fmt);

	for (const char *p = fmt; *p; p++) {
		if (*p != '%') {
			write(STDOUT_FILENO, p, 1);
			continue;
		}
		p++;
		switch (*p) {
		case 'd': {
			long v = __builtin_va_arg(ap, long);
			uputint(v);
			break;
		}
		case 's': {
			const char *s = __builtin_va_arg(ap, const char *);
			uputs(s ? s : "(null)");
			break;
		}
		case 'x': {
			unsigned long v = __builtin_va_arg(ap, unsigned long);
			uputhex(v);
			break;
		}
		case 'c': {
			char c = (char)__builtin_va_arg(ap, int);
			uputc(c);
			break;
		}
		case '%':
			uputc('%');
			break;
		default:
			uputc('%');
			if (*p)
				uputc(*p);
			break;
		}
	}

	__builtin_va_end(ap);
}

// 从标准输入读一行 (最多 len-1 字节), 返回实际读到的字符数。
// 内核此处是行缓冲控制台: 无数据时 read 返回 0, 故须循环重试; 无输入时会一直空转。
unsigned long ugets(char *buf, unsigned long len)
{
	unsigned long n = 0;

	if (len == 0)
		return 0;

	while (n + 1 < len) {
		char c;
		long r = read(STDIN_FILENO, &c, 1);
		if (r <= 0)
			continue; /* 暂时没有数据: 重试 */
		if (c == '\n') {
			uputc('\n');
			break;
		}
		buf[n++] = c;
		// 回显在用户态做 (内核只管行缓冲)。
		uputc(c);
	}

	buf[n] = '\0';
	return n;
}
