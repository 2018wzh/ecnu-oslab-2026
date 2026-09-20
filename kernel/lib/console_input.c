// 教师提供的行编辑缓冲。UART 中断只生产，进程上下文消费。
#include <kernel/console.h>
#include <kernel/proc.h>
static struct { spinlock_t lock; uint8 data[128]; uint64 read, write, edit; } input;
void console_input_init(void) { spinlock_init(&input.lock, "console input"); }
void console_input(uint8 c)
{
    spinlock_acquire(&input.lock);
    if (c == 21) { // Ctrl-U
        while (input.edit != input.write && input.data[(input.edit - 1) % 128] != '\n') {
            --input.edit; console_putc('\b'); console_putc(' '); console_putc('\b');
        }
    } else if (c == 8 || c == 127) {
        if (input.edit != input.write) { --input.edit; console_putc('\b'); console_putc(' '); console_putc('\b'); }
    } else if (c && input.edit - input.read < 128) {
        if (c == '\r') c = '\n';
        if (c != 4) console_putc(c);
        input.data[input.edit++ % 128] = c;
        if (c == '\n' || c == 4 || input.edit - input.read == 128) {
            input.write = input.edit; proc_wakeup(&input.read);
        }
    }
    spinlock_release(&input.lock);
}
long console_read(void *dst, size_t len)
{
    size_t n = 0; uint8 *out = dst;
    spinlock_acquire(&input.lock);
    while (n < len) {
        while (input.read == input.write) proc_sleep(&input.read, &input.lock);
        uint8 c = input.data[input.read++ % 128];
        if (c == 4) { if (n) --input.read; break; }
        out[n++] = c; if (c == '\n') break;
    }
    spinlock_release(&input.lock); return n;
}
