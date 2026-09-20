/* 标准输出和报错机制；数值转换与紧急输出由教师提供。 */
#include <stdarg.h>
#include <kernel/print.h>
#include <kernel/console.h>
#include <kernel/lock.h>
#include <kernel/arch.h>
/* 整次 printf 共用的自旋锁。 */
spinlock_t print_lock;
/* 初始化 UART 和打印锁；并发使用前由主核调用一次。 */
void print_init(void) { console_init(); spinlock_init(&print_lock, "print"); }
/* %x：无符号十六进制；也供十进制转换调用，base 为 10 或 16。 */
void print_unsigned(uint64 value, unsigned base)
{
    const char *digits = "0123456789abcdef";
    char buf[64]; unsigned n = 0;
    do { buf[n++] = digits[value % base]; value /= base; } while (value);
    while (n) console_putc(buf[--n]);
}
/* %d：32 位有符号十进制，包括最小负整数。 */
void print_signed(int value)
{
    uint32 magnitude = (uint32)value;
    if (value < 0) { console_putc('-'); magnitude = 0U - magnitude; }
    print_unsigned(magnitude, 10);
}
/* %p：0x 前缀和完整 16 位十六进制，保留前导零。教师辅助函数。 */
void print_pointer(const void *pointer)
{
    uint64 value = (uint64)(uintptr_t)pointer;
    const char *digits = "0123456789abcdef";
    console_putc('0');
    console_putc('x');
    for (unsigned i = 0; i < 16; ++i, value <<= 4)
        console_putc(digits[value >> 60]);
}
/* TODO(lab-1): 完成格式分派，锁覆盖整次输出。
 * %d：int，32 位有符号十进制；%x：unsigned int，32 位无符号十六进制。
 * %p：void *，64 位指针，0x 加 16 位十六进制。
 * %c：默认提升为 int 的字符；%s：const char * 字符串。
 * 提示：stdarg.h 中的 va_list 用于访问可变参数。
 * NULL 字符串显示为 (null) 是选做。
 */
void printf(const char *fmt, ...)
{ (void)fmt; panic("TODO(lab-1): printf"); }
// TODO(lab-1): 条件失败时报告 warning 并停止。
void assert(bool condition, const char *warning)
{ (void)condition; (void)warning; panic("TODO(lab-1): assert"); }
void panic(const char *message)
{
    arch_irq_disable();
    emergency_puts("\nPANIC: "); emergency_puts(message); emergency_puts("\n");
    arch_park();
}
