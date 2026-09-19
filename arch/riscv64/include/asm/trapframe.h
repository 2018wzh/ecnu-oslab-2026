// 陷入时的寄存器现场 (架构相关)。
// CPU 从用户态陷入内核时, 用户态寄存器必须完整保存, 处理完才能恢复现场,
// trapframe 就是内核栈上保存这些寄存器的区域。
// 字段和顺序直接决定汇编偏移 (RISC-V x1..x31, AArch64 x0..x30 不同, 不能共用),
// 所以放 arch 层。最易错: C 结构体与汇编偏移必须一致, 插入字段汇编不报错但会
// 保存/恢复到错寄存器, 表现为"系统调用偶尔返回错值"。折中: 字段顺序在注释标
// 号、STATIC_ASSERT 校验总大小、汇编用符号偏移不用裸数字。
#ifndef __ASM_TRAPFRAME_H__
#define __ASM_TRAPFRAME_H__

/* 本头文件会被汇编 (.S) 引用, 而 kernel/types.h 是纯 C 的。
 * 因此必须区分两种语境: 汇编只关心下面的偏移宏, 不需要类型定义。 */
#ifndef __ASSEMBLER__
#include <kernel/types.h>
#endif

/* 通用寄存器 x1..x31 加上 sepc。
 * x0 (zero) 不需要保存, 它恒为 0。
 *
 * 字段顺序必须与 trap.S 中的 STORE/LOAD 顺序严格一致。
 * 索引即寄存器号: regs[0] = x1(ra), regs[1] = x2(sp), ... regs[30] = x31(t6)
 */
#ifndef __ASSEMBLER__
struct trapframe {
	uint64 regs[31];    /* x1..x31 */
	uint64 sepc;        /* 触发 trap 的 PC */
	uint64 kernel_sp;   /* 本进程的内核栈顶 (供调试观察) */
	uint64 kernel_tp;   /* 进入用户态前那个 hart 的 tp (= hartid) */
};

/* 编译期校验: 结构体大小必须与下面的偏移宏定义一致。
 * 如果有人往结构体里加了字段却忘了改 TF_* 宏 (或反之),
 * 编译会立刻失败, 而不是等到运行时才发现寄存器保存错位。 */
STATIC_ASSERT(sizeof(struct trapframe) == 34 * 8,
              "trapframe 大小与 TF_* 偏移宏不一致, 请同步更新");

/* --------------------------------------------------------------------------
 * 按语义命名的存取宏 (给 **generic kernel** 用)
 * --------------------------------------------------------------------------
 * 上面那一组 TF_* 是**字节偏移**, 给汇编与 STATIC_ASSERT 用;
 * 下面这一组是**函数式宏**, 读写结构体字段。
 *
 * 分成两组的原因: generic kernel 不应该写 `tf->sepc` 或 `tf->regs[9]` ——
 * 那些是 RISC-V 的寄存器名 (sepc、x10=a0), 换架构就变。它只应该说
 * "把这个现场的执行地址设为 entry"、"把返回值设成 0", 也就是用下面
 * 这些名字。字面量到字段的那一层映射留在 arch 层。
 * -------------------------------------------------------------------------- */

/* 触发 trap / 继续执行的指令地址。 */
#define ARCH_TF_PC(tf)             ((tf)->sepc)
#define ARCH_TF_SET_PC(tf, v)      ((tf)->sepc = (uint64)(v))

/* 用户栈指针 (x2 = sp)。 */
#define ARCH_TF_USER_SP(tf)        ((tf)->regs[1])
#define ARCH_TF_SET_USER_SP(tf, v) ((tf)->regs[1] = (uint64)(v))

/* 返回值寄存器 (x10 = a0): 系统调用的返回值, 以及 fork 的"子进程
 * 返回 0", 都靠它。 */
#define ARCH_TF_RET(tf)            ((tf)->regs[9])
#define ARCH_TF_SET_RET(tf, v)     ((tf)->regs[9] = (uint64)(v))

/* 把整个现场清零 (exec 换程序时用: 新程序不该读到旧程序留下的寄存器)。
 * 逐字节写而不是 memset —— 本头文件也会被汇编包含, 不依赖 C 库。 */
#define ARCH_TF_ZERO(tf)                                                  \
	do {                                                              \
		uint8 *_p = (uint8 *)(tf);                                \
		for (uint64 _i = 0; _i < sizeof(struct trapframe); _i++)  \
			_p[_i] = 0;                                       \
	} while (0)
#endif /* __ASSEMBLER__ */

/* 各字段在结构体中的字节偏移, 供汇编使用 */
#define TF_RA      (0 * 8)
#define TF_SP      (1 * 8)
#define TF_GP      (2 * 8)
#define TF_TP      (3 * 8)
#define TF_T0      (4 * 8)
#define TF_T1      (5 * 8)
#define TF_T2      (6 * 8)
#define TF_S0      (7 * 8)
#define TF_S1      (8 * 8)
#define TF_A0      (9 * 8)
#define TF_A1      (10 * 8)
#define TF_A2      (11 * 8)
#define TF_A3      (12 * 8)
#define TF_A4      (13 * 8)
#define TF_A5      (14 * 8)
#define TF_A6      (15 * 8)
#define TF_A7      (16 * 8)
#define TF_S2      (17 * 8)
#define TF_S3      (18 * 8)
#define TF_S4      (19 * 8)
#define TF_S5      (20 * 8)
#define TF_S6      (21 * 8)
#define TF_S7      (22 * 8)
#define TF_S8      (23 * 8)
#define TF_S9      (24 * 8)
#define TF_S10     (25 * 8)
#define TF_S11     (26 * 8)
#define TF_T3      (27 * 8)
#define TF_T4      (28 * 8)
#define TF_T5      (29 * 8)
#define TF_T6      (30 * 8)
#define TF_SEPC    (31 * 8)
#define TF_KSP     (32 * 8)
#define TF_KTP     (33 * 8)
#define TF_SIZE    (34 * 8)

#endif /* __ASM_TRAPFRAME_H__ */
