// 内核格式化输出 (generic kernel)。
// 本文件不知道底下是什么串口: 只调 console_putc(), 由 generic console 层再调
// 具体 UART 驱动, 所以换串口硬件不用改这里的任何格式化代码。
#include <kernel/types.h>
#include <kernel/string.h>
#include <kernel/print.h>
#include <kernel/sync.h>

volatile int panicked = 0;

static spinlock_t print_lock;

void print_init(void)
{
        spinlock_init(&print_lock, "pr");
}
static void print_num(uint64 v, int base, int sign, int width,
                      int zero_pad, int left)
{
}

static void vprintf_impl(const char *fmt, __builtin_va_list ap)
{
        for (const char *p = fmt; *p; p++) {
                if (*p != '%') {
                        console_putc(*p);
                        continue;
                }

                p++;

                /* ---- 解析标志位 ---- */
                int zero_pad = 0, left = 0;
                for (;; p++) {
                        if (*p == '0') zero_pad = 1;
                        else if (*p == '-') left = 1;
                        else break;
                }
                /* 左对齐时忽略零填充 (标准 printf 语义) */
                if (left)
                        zero_pad = 0;

                /* ---- 解析宽度 ---- */
                int width = 0;
                while (*p >= '0' && *p <= '9') { width = width * 10 + (*p - '0'); p++; }

                /* ---- 解析精度 (目前只用于 %s 的字符串截断) ---- */
                int precision = -1;
                if (*p == '.') {
                        p++;
                        precision = 0;
                        while (*p >= '0' && *p <= '9') {
                                precision = precision * 10 + (*p - '0');
                                p++;
                        }
                }

                /* 跳过长度修饰符 (我们统一按 64 位处理) */
                while (*p == 'l' || *p == 'z') p++;

                switch (*p) {
                case 'd': case 'i':
                        print_num((uint64)__builtin_va_arg(ap, int), 10, 1, width, zero_pad, left);
                        break;
                case 'u':
                        print_num((uint64)__builtin_va_arg(ap, unsigned int), 10, 0, width, zero_pad, left);
                        break;
                case 'x': case 'X':
                        print_num(__builtin_va_arg(ap, uint64), 16, 0, width, zero_pad, left);
                        break;
                case 'p':
                        console_putc('0'); console_putc('x');
                        print_num(__builtin_va_arg(ap, uint64), 16, 0, 16, 1, 0);
                        break;
                case 'c':
                        console_putc(__builtin_va_arg(ap, int));
                        break;
                case 's': {
                        const char *s = __builtin_va_arg(ap, const char *);
                        if (!s) s = "(null)";
                        /* 计算要输出的长度 (受精度限制) */
                        int len = 0;
                        while (s[len] && (precision < 0 || len < precision))
                                len++;
                        /* 左对齐时先输出再补空格 */
                        for (int k = 0; k < len; k++)
                                console_putc(s[k]);
                        if (left)
                                for (int k = len; k < width; k++)
                                        console_putc(' ');
                        break;
                }
                case '%':
                        console_putc('%');
                        break;
                default:
                        /* 未知格式符按原样输出, 便于发现问题 */
                        console_putc('%');
                        if (*p) console_putc(*p);
                        break;
                }
        }
}

void printf(const char *fmt, ...)
{
        spinlock_acquire(&print_lock);
        __builtin_va_list ap;
        __builtin_va_start(ap, fmt);
        vprintf_impl(fmt, ap);
        __builtin_va_end(ap);
        spinlock_release(&print_lock);
}

void panic(const char *fmt, ...)
{
        panicked = 1;   /* 通知 UART 驱动停止等待, 保证崩溃信息能输出 */

        console_putc_raw('\n');
        console_putc_raw('P');
        console_putc_raw('A');
        console_putc_raw('N');
        console_putc_raw('I');
        console_putc_raw('C');
        console_putc_raw(':');
        console_putc_raw(' ');

        __builtin_va_list ap;
        __builtin_va_start(ap, fmt);
        vprintf_impl(fmt, ap);
        __builtin_va_end(ap);

        console_putc_raw('\n');

        for (;;)
                asm volatile("wfi");
}
