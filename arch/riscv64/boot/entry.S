// 内核入口 (S-mode, 由 OpenSBI/U-Boot 进入)。
// 进入时: a0=hartid, a1=dtb(不用), satp=0, sp 未定义, sstatus.SIE=0。
// 本文件做三件事, 顺序不能错:
//   1. 建栈 (栈可用前不能执行任何 C 代码)
//   2. 保存 hartid 到 tp (S-mode 读 mhartid 会非法指令, 只能现在从 a0 拿)
//   3. 跳到 C 的 start()
#include <platform.h>
#include <asm/cold_boot.h>

.section .text.entry
.global _entry
_entry:
        /* ---- 1. 建立栈 ----
         * 每个 hart 有独立的内核栈, 布局如下:
         *   boot_stack: [hart0 的栈][hart1 的栈][hart2 的栈]...
         *   sp = boot_stack + (hartid + 1) * STACK_SIZE
         * 即 sp 指向"本 hart 那一格"的栈顶。
         *
         * 用 (hartid + 1) 而不是 hartid, 是因为 hartid 可能是 0,
         * 那样 sp 会指向 boot_stack 起点, 压栈时立刻写到数组外面去。
         *
         * 为什么不在 C 里建立栈:
         *   编译器在函数入口就会使用 sp (保存 ra、分配局部变量),
         *   所以建立栈必须在任何 C 代码之前, 只能用汇编做。
         */
        la      sp, boot_stack
        li      t0, 4096                /* 每核栈大小 = 1 页 */
        li      t1, PLAT_HART_MIN
        bltu    a0, t1, .Lpark          /* hartid < 最小值 -> 非法 */
        li      t1, PLAT_HART_MAX
        bgtu    a0, t1, .Lpark          /* hartid > 最大值 -> 非法 */

        /* sp = boot_stack + (hartid - PLAT_HART_MIN + 1) * 4096 */
        li      t1, PLAT_HART_MIN
        sub     t2, a0, t1              /* t2 = 核内序号 (从 0 开始) */
        addi    t2, t2, 1               /* +1, 保证 sp 指向栈顶而非栈底 */
        mul     t0, t0, t2
        add     sp, sp, t0

        /* ---- 2. 保存 hartid 到 tp ----
         * 之后所有代码都用 arch_read_tp() 获取 hartid,
         * 因为 S-mode 无法读取 mhartid。 */
        mv      tp, a0
        la      t0, __cold_boot_lottery
        li      t1, 1
        amoswap.w t2, t1, (t0)          /* t2 = 交换前的值 (32 位, 带符号扩展) */
        slli    t2, t2, 32
        srli    t2, t2, 32
        li      t3, LOTTERY_FREE
        beq     t2, t3, .Lwinner        /* 还是哨兵 -> 我是第一个 */
        beqz    t2, .Lwinner            /* 是 0 -> 哨兵没被加载, 也当第一个 */
#ifdef CONFIG_BOOT_LOSERS_PARK
        /* 没有固件 (裸机直启): 其他 hart 是固件/模拟器直接放进来的,
         * 而本协议没有 HSM 去"正式启动"它们 —— 所以让它们停车, 不要
         * 进内核。有固件时相反: 输家会被内核用 HSM 正式启动, 那时它
         * 走的是 secondary 路径, 见 mk/boot/sbi.mk 的说明。 */
        j       .Lpark
#endif
        j       .Lwait_claim            /* 别人先认领了, 去等它写下身份 */

.Lwinner:
        /* 先记身份 (hartid + 1: 0 专门表示"还没认领", 与"启动核恰好是
         * hart 0"区分开), 再清 .bss。身份存放在 .data, 不在被清的范围内,
         * 所以任何后来者读到的一定是有效值。 */
        la      t0, g_cold_boot_hartid
        addi    t1, a0, 1
        sd      t1, 0(t0)

        la      t0, __bss_start
        la      t1, __bss_end
.Lbss_loop:
        bgeu    t0, t1, .Lskip_bss
        sd      zero, 0(t0)
        addi    t0, t0, 8
        j       .Lbss_loop

.Lwait_claim:
        /* 输家: 等赢家写下身份再走。窗口只有两条指令, 但不等的话可能读到
         * 0, 于是"抽签没发生"被当成事实。计数上限是给"哨兵是垃圾值、
         * 谁都没赢"这种病态情况留的出路 —— 那时与其静默死等, 不如让
         * 后面的检查报一条明确的错误。 */
        la      t0, g_cold_boot_hartid
        li      t1, CLAIM_SPIN
.Lwait_loop:
        ld      t2, 0(t0)
        bnez    t2, .Lskip_bss
        addi    t1, t1, -1
        bnez    t1, .Lwait_loop

.Lskip_bss:

        /* ---- 3. 跳转到 C 代码 ---- */
        call    start

        /* start() 正常情况下不会返回 (它会进入 main 的死循环)。
           如果返回了, 说明出了严重问题, 让该 hart 停下。 */
.Lpark:
        wfi
        j       .Lpark

/* --------------------------------------------------------------------------
 * 每个 hart 的启动栈。
 * 放在 .bss 段 (不占镜像空间), 并且按 16 字节对齐 ——
 * RISC-V ABI 要求栈指针 16 字节对齐。
 * -------------------------------------------------------------------------- */
.section .bss
.align 4
.global boot_stack
boot_stack:
        .space 4096 * 8    /* 最多支持 8 个 hart, 足够涵盖 QEMU 与 VF2 */

/* --------------------------------------------------------------------------
 * 冷启动核抽签用的两个符号 (见前面第 2.5 步)
 * -------------------------------------------------------------------------- */

/* 哨兵: 必须在 **.data** —— 它要在 .bss 被清零之前可读, 而 .bss 在
 * 裸二进制加载路径上装的是内存里的垃圾。.data 有文件内容, 两条加载
 * 路径 (QEMU 的 ELF / U-Boot 的裸二进制) 都会把它搬进来。 */
.section .data
.align 3
.global __cold_boot_lottery
__cold_boot_lottery:
        .quad 0xB007B007
.section .data
.align 3
.global g_cold_boot_hartid
g_cold_boot_hartid:
        .quad 0
