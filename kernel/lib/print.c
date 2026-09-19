#include <stdarg.h>
#include <kernel/print.h>
#include <kernel/console.h>
#include <kernel/lock.h>
#include <kernel/arch.h>
spinlock_t print_lock;
void print_init(void) { console_init(); spinlock_init(&print_lock, "print"); }
void print_unsigned(uint64 value, unsigned base)
{
    const char *digits = "0123456789abcdef";
    char buf[64]; unsigned n = 0;
    do { buf[n++] = digits[value % base]; value /= base; } while (value);
    while (n) console_putc(buf[--n]);
}
void print_signed(int value)
{
    uint32 magnitude = (uint32)value;
    if (value < 0) { console_putc('-'); magnitude = 0U - magnitude; }
    print_unsigned(magnitude, 10);
}
// TODO(lab-1): 支持 %d %x %p %c %s %%；锁覆盖整次输出。
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
