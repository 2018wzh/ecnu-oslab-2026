/* 16550 兼容 UART 驱动: 地址与时钟全部来自 <platform.h>, 两个平台一份源码。 */

#include <kernel/types.h>
#include <kernel/print.h>
#include <platform.h>

/* ---- 16550 寄存器偏移 ---- */
#define UART_RHR 0   /* 读: 接收数据 */
#define UART_THR 0   /* 写: 发送数据 */
#define UART_IER 1   /* 中断使能 */
#define UART_FCR 2   /* FIFO 控制 */
#define UART_LCR 3   /* 线路控制 */
#define UART_LSR 5   /* 线路状态 */

/* LSR 位 */
#define UART_LSR_RX_READY (1 << 0)   /* 有数据可读 */
#define UART_LSR_TX_IDLE  (1 << 5)   /* 发送寄存器空 */

/* LCR 位 */
#define UART_LCR_8BITS     (3 << 0)  /* 8 位数据 */
#define UART_LCR_BAUD_LATCH (1 << 7) /* 允许写分频寄存器 */

/* FIFO 控制 */
#define UART_FCR_ENABLE (1 << 0)
#define UART_FCR_CLEAR  (3 << 1)

/* ---- 寄存器访问 ----
 * volatile 保证每次访问都真读写内存, 否则编译器会把轮询循环优化成死循环。 */
#define UART_REG(off) ((volatile uint8 *)(PLAT_UART0_BASE + (off)))
#define UART_READ(off)  (*(UART_REG(off)))
#define UART_WRITE(off, v) (*(UART_REG(off)) = (uint8)(v))

// 由 console 层提供: 收到字符后交给它处理 (回显、行编辑等)。
extern void console_intr(int c);
extern volatile int panicked;

// 初始化: 关闭中断、设波特率、8 位数据、清空并使能 FIFO。
void uart_init(void)
{
	// 关闭所有中断: 本课程用轮询, 不依赖 UART 中断。
	UART_WRITE(UART_IER, 0x00);

	// 进入分频设置模式, 写入分频值 = 输入时钟 / (16 * 波特率)。
	// 注意: 算错会表现为串口输出乱码, 是移植到新板子时的常见首问。
	UART_WRITE(UART_LCR, UART_LCR_BAUD_LATCH);
	uint32 divisor = PLAT_UART0_CLOCK / (16 * 115200);
	UART_WRITE(0, divisor & 0xFF);          /* 分频低 8 位 */
	UART_WRITE(1, (divisor >> 8) & 0xFF);   /* 分频高 8 位 */

	// 8 位数据, 无校验, 1 位停止位。
	UART_WRITE(UART_LCR, UART_LCR_8BITS);

	// 清空并使能 FIFO。
	UART_WRITE(UART_FCR, UART_FCR_ENABLE | UART_FCR_CLEAR);
}

// 输出一个字符 (轮询, 阻塞)。
void uart_putc(int c)
{
	// 内核崩溃后什么都不可信, panic 时不再回显。
	while (panicked)
		;

	// 等待发送寄存器空闲: 不等就写会覆盖前一个字符, 造成丢字。
	while ((UART_READ(UART_LSR) & UART_LSR_TX_IDLE) == 0)
		;

	UART_WRITE(UART_THR, c);
}

// 读取一个字符 (轮询, 非阻塞), 无数据返回 -1。
int uart_getc(void)
{
	if (UART_READ(UART_LSR) & UART_LSR_RX_READY)
		return (int)UART_READ(UART_RHR);
	return -1;
}

// 中断处理入口: 本课程用轮询, 保留以支持将来启用中断模式。
void uart_intr(void)
{
	while (1) {
		int c = uart_getc();
		if (c == -1)
			break;
		console_intr(c);
	}
}
