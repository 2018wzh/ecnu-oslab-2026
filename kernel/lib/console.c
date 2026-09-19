// 控制台抽象层 (generic kernel): printf 与串口驱动之间的隔离带。
// printf -> console_putc() -> uart_putc() -> 平台地址。
// 还负责行缓冲与基本行编辑 (退格/回车), 与硬件无关, 换串口不用动它。
#include <kernel/types.h>
#include <kernel/print.h>
#include <kernel/sync.h>

// 由 drivers/serial/ 提供
void uart_init(void);
void uart_putc(int c);
int  uart_getc(void);
void uart_intr(void);

#define CONSOLE_BUF_SIZE 128

static struct {
        spinlock_t lk;
        char buf[CONSOLE_BUF_SIZE];
        int read_idx;
        int write_idx;
        int edit_idx;
} cons;

void console_init(void)
{
        spinlock_init(&cons.lk, "cons");
        cons.read_idx = 0;
        cons.write_idx = 0;
        cons.edit_idx = 0;
        uart_init();
}

// 底层输出: 直接走 UART, 不加锁 (供 panic 使用)
void console_putc_raw(int c)
{
        uart_putc(c);
}

void console_putc(int c)
{
        uart_putc(c);
}

// 收到一个输入字符后的处理: 行编辑
void console_intr(int c)
{
        switch (c) {
        case '\r':
        case '\n':
                // 回车: 把当前行提交到读缓冲
                cons.buf[cons.edit_idx] = '\n';
                cons.edit_idx++;
                cons.write_idx = cons.edit_idx;
                console_putc('\n');
                break;
        case 0x7f:  // 退格
        case '\b':
                if (cons.edit_idx > 0) {
                        cons.edit_idx--;
                        console_putc('\b');
                        console_putc(' ');
                        console_putc('\b');
                }
                break;
        default:
                if (c >= 32 && c < 127 && cons.edit_idx < CONSOLE_BUF_SIZE - 1) {
                        cons.buf[cons.edit_idx++] = (char)c;
                        console_putc(c);   // 回显
                }
                break;
        }
}

// 读取一个字符 (轮询)。返回 -1 表示暂无输入。
int console_getc(void)
{
        int c = uart_getc();
        if (c >= 0)
                console_intr(c);

        if (cons.read_idx < cons.write_idx) {
                return (int)(unsigned char)cons.buf[cons.read_idx++];
        }
        return -1;
}
