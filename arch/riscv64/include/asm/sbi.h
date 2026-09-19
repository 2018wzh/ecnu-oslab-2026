// SBI (Supervisor Binary Interface) 固件调用。
// 这是"固件接口"层, 与 arch 层并列但职责不同: arch 管 CPU 自身机制 (CSR/页表/
// 上下文), firmware 管运行期可请求固件做的事 (打印/定时器/启动其他 hart/复位)。
// generic kernel 不直接调 sbi_ 函数, 只调 include/kernel/ 里的 cpu_start() 等,
// 由 arch 层决定底层是 SBI 还是别的机制, 换 loader 不用改 generic。
// 调用约定 (同 Linux): a7=EID, a6=FID, a0-a5=参数; 返回 a0=错误码, a1=返回值;
// 经 ecall 从 S-mode 陷入 M-mode 固件。
#ifndef __ASM_SBI_H__
#define __ASM_SBI_H__

#include <kernel/types.h>

struct sbiret {
	long error;
	long value;
};

/* --------------------------------------------------------------------------
 * 1. 底层 ecall 封装
 * -------------------------------------------------------------------------- */
static inline struct sbiret sbi_ecall(uint64 eid, uint64 fid,
                                      uint64 a0, uint64 a1, uint64 a2,
                                      uint64 a3, uint64 a4, uint64 a5)
{
	register uint64 r_a0 asm("a0") = a0;
	register uint64 r_a1 asm("a1") = a1;
	register uint64 r_a2 asm("a2") = a2;
	register uint64 r_a3 asm("a3") = a3;
	register uint64 r_a4 asm("a4") = a4;
	register uint64 r_a5 asm("a5") = a5;
	register uint64 r_a6 asm("a6") = fid;
	register uint64 r_a7 asm("a7") = eid;

	asm volatile("ecall"
	             : "+r"(r_a0), "+r"(r_a1)
	             : "r"(r_a2), "r"(r_a3), "r"(r_a4), "r"(r_a5),
	               "r"(r_a6), "r"(r_a7)
	             : "memory");

	return (struct sbiret){ .error = (long)r_a0, .value = (long)r_a1 };
}

/* --------------------------------------------------------------------------
 * 2. 扩展 ID 定义
 * -------------------------------------------------------------------------- */

/* 旧版 (Legacy) 控制台扩展: 只有两个功能, 不带返回值语义。
 * 仍然被所有固件支持, 教学上最直观。 */
#define SBI_EXT_LEGACY_PUTCHAR  0x01
#define SBI_EXT_LEGACY_GETCHAR  0x02

/* 新版扩展 (带错误码) */
#define SBI_EXT_TIME            0x54494D45  /* "TIME"  定时器 */
#define SBI_EXT_IPI             0x00735049  /* "sPI"   核间中断 */
#define SBI_EXT_RFENCE          0x52464E43  /* "RFNC"  远程 TLB 刷新 */
#define SBI_EXT_HSM             0x48534D    /* "HSM"   hart 状态管理 */
#define SBI_EXT_SRST            0x53525354  /* "SRST"  系统复位 */
#define SBI_EXT_DBCN            0x4442434E  /* "DBCN"  调试控制台 */

/* TIME 扩展功能 */
#define SBI_TIME_SET_TIMER      0

/* RFENCE 扩展功能 */
#define SBI_RFENCE_SFENCE_VMA   1

/* HSM 扩展功能 */
#define SBI_HSM_HART_START      0
#define SBI_HSM_HART_STOP       1
#define SBI_HSM_HART_GET_STATUS 2

/* SRST 扩展功能 */
#define SBI_SRST_RESET          0
#define SBI_SRST_RESET_TYPE_SHUTDOWN 0
#define SBI_SRST_RESET_TYPE_COLD     1
#define SBI_SRST_RESET_REASON_NONE   0

/* DBCN 扩展功能 (新版调试控制台, 支持一次写多个字符) */
#define SBI_DBCN_CONSOLE_WRITE  0
#define SBI_DBCN_CONSOLE_READ   1
#define SBI_DBCN_CONSOLE_WRITE_BYTE 2

/* --------------------------------------------------------------------------
 * 3. 常用封装
 * -------------------------------------------------------------------------- */

/* 输出一个字符到固件控制台 (阻塞直到写出)。
 * 这是内核最早期的输出手段: 在 UART 驱动初始化之前就能用,
 * 所以调试启动阶段的问题时非常关键。 */
static inline void sbi_console_putchar(int ch)
{
	sbi_ecall(SBI_EXT_LEGACY_PUTCHAR, 0, (uint64)ch, 0, 0, 0, 0, 0);
}

/* 读取一个字符, 无字符时返回 -1 */
static inline int sbi_console_getchar(void)
{
	struct sbiret r = sbi_ecall(SBI_EXT_LEGACY_GETCHAR, 0, 0, 0, 0, 0, 0, 0);
	return (int)r.error;
}

/* 设置下一次时钟中断的绝对时间 (单位: rdtime 的 tick)。
 * 注意: 这是"绝对时间"而不是"间隔"。传相对量会导致中断不再触发。 */
static inline void sbi_set_timer(uint64 stime_value)
{
	sbi_ecall(SBI_EXT_TIME, SBI_TIME_SET_TIMER, stime_value, 0, 0, 0, 0, 0);
}

/* 启动另一个 hart, 让它从 start_addr 开始执行。
 * opaque 会作为 a1 传给那个 hart (Linux 用它传 hartid)。 */
static inline int sbi_hart_start(uint64 hartid, uint64 start_addr, uint64 opaque)
{
	struct sbiret r = sbi_ecall(SBI_EXT_HSM, SBI_HSM_HART_START,
	                            hartid, start_addr, opaque, 0, 0, 0);
	return (int)r.error;
}

/* 停止当前 hart */
static inline void sbi_hart_stop(void)
{
	sbi_ecall(SBI_EXT_HSM, SBI_HSM_HART_STOP, 0, 0, 0, 0, 0, 0);
}

/* 关闭整个系统 */
static inline void sbi_system_shutdown(void)
{
	sbi_ecall(SBI_EXT_SRST, SBI_SRST_RESET,
	          SBI_SRST_RESET_TYPE_SHUTDOWN, SBI_SRST_RESET_REASON_NONE,
	          0, 0, 0, 0);
}

/* 远程刷新 TLB。当修改了其他 hart 正在使用的页表时, 必须调用它,
 * 否则那些 hart 可能继续使用过期的 TLB 条目 —— 这是多核内核
 * 非常经典的一类 bug。 */
static inline void sbi_remote_sfence_vma(uint64 hart_mask, uint64 start, uint64 size)
{
	sbi_ecall(SBI_EXT_RFENCE, SBI_RFENCE_SFENCE_VMA,
	          hart_mask, 0, start, size, 0, 0);
}

#endif /* __ASM_SBI_H__ */
