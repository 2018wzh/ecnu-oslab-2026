// RISC-V 控制状态寄存器 (CSR) 访问。
// CSR 是 RISC-V ISA 特有机制 (换 aarch64 是系统寄存器 + MRS/MSR), 所以放
// arch/riscv64/ 下; generic kernel 不知道 satp/sstatus/mhartid, 只能经
// include/kernel/ 的抽象接口间接用。本文件是这些知识的唯一存放处。
#ifndef __ASM_CSR_H__
#define __ASM_CSR_H__

#include <kernel/types.h>

/* --------------------------------------------------------------------------
 * 1. CSR 读写原语
 * -------------------------------------------------------------------------- */

#define READ_CSR(reg)                                                          \
	({                                                                     \
		uint64 __v;                                                    \
		asm volatile("csrr %0, " #reg : "=r"(__v) : : "memory");       \
		__v;                                                           \
	})

#define WRITE_CSR(reg, val)                                                    \
	do {                                                                   \
		uint64 __v = (uint64)(val);                                    \
		asm volatile("csrw " #reg ", %0" : : "r"(__v) : "memory");     \
	} while (0)

#define SET_CSR(reg, bits)                                                     \
	do {                                                                   \
		uint64 __v = (uint64)(bits);                                   \
		asm volatile("csrs " #reg ", %0" : : "r"(__v) : "memory");     \
	} while (0)

#define CLEAR_CSR(reg, bits)                                                   \
	do {                                                                   \
		uint64 __v = (uint64)(bits);                                   \
		asm volatile("csrc " #reg ", %0" : : "r"(__v) : "memory");     \
	} while (0)

/* --------------------------------------------------------------------------
 * 3. CSR 位定义
 * -------------------------------------------------------------------------- */

/* sstatus */
#define SSTATUS_SIE   (1UL << 1)   /* S-mode 中断使能 */
#define SSTATUS_SPIE  (1UL << 5)   /* 进入 trap 前的中断使能状态 */
#define SSTATUS_SPP   (1UL << 8)   /* 进入 trap 前的特权级: 1=S, 0=U */
#define SSTATUS_SUM   (1UL << 18)  /* 允许 S-mode 访问 U 页面的数据 */

/* sie / sip */
#define SIE_SSIE (1UL << 1)        /* 软件中断 */
#define SIE_STIE (1UL << 5)        /* 时钟中断 */
#define SIE_SEIE (1UL << 9)        /* 外部中断 */

/* scause: 异常/中断原因。
 * 最高位为 1 表示中断, 为 0 表示异常。 */
#define SCAUSE_INTERRUPT (1UL << 63)
#define SCAUSE_CODE_MASK (~(1UL << 63))

#define SCAUSE_S_SOFTWARE   1
#define SCAUSE_S_TIMER      5
#define SCAUSE_S_EXTERNAL   9

/* 常见的异常 */
#define SCAUSE_INST_PAGE_FAULT   12
#define SCAUSE_LOAD_PAGE_FAULT   13
#define SCAUSE_STORE_PAGE_FAULT  15
#define SCAUSE_ECALL_FROM_U      8
#define SCAUSE_ECALL_FROM_S      9

/* sstatus 的 MPP 字段 (仅在从 M-mode 进入时有意义) */
#define SSTATUS_MPP_MASK (3UL << 11)
#define SSTATUS_MPP_S    (1UL << 11)
#define SSTATUS_MPP_U    (0UL << 11)

/* --------------------------------------------------------------------------
 * 4. 常用 CSR 的直接访问封装
 *    generic kernel 不直接调用这些函数, 它们只被 arch 层内部使用。
 * -------------------------------------------------------------------------- */

static inline uint64 arch_read_satp(void)  { return READ_CSR(satp); }
static inline void   arch_write_satp(uint64 v) { WRITE_CSR(satp, v); }

static inline uint64 arch_read_stvec(void) { return READ_CSR(stvec); }
static inline void   arch_write_stvec(uint64 v) { WRITE_CSR(stvec, v); }

static inline uint64 arch_read_scause(void) { return READ_CSR(scause); }
static inline uint64 arch_read_sepc(void)   { return READ_CSR(sepc); }
static inline uint64 arch_read_stval(void)  { return READ_CSR(stval); }

static inline void arch_write_sepc(uint64 v) { WRITE_CSR(sepc, v); }
static inline uint64 arch_read_tp(void)
{
	uint64 v;
	asm volatile("mv %0, tp" : "=r"(v));
	return v;
}

static inline void arch_write_tp(uint64 v)
{
	asm volatile("mv tp, %0" : : "r"(v));
}


/* --------------------------------------------------------------------------
 * 5. 内存屏障
 * -------------------------------------------------------------------------- */



#endif /* __ASM_CSR_H__ */
