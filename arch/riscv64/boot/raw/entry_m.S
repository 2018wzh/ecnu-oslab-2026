// 裸机直启: M-mode -> S-mode 的过渡。无固件时 QEMU `-bios none -kernel`
// 在 M-mode 直接跳到入口, 于是 M-mode 的准备工作须由内核自己做:
//   1. PMP   不配则 S-mode 一条指令都取不到
//   2. 委派  medeleg/mideleg 决定异常/中断是否交给 S-mode
//   3. Sstc  让 S-mode 用自己的 stimecmp 直接设时钟
//   4. mret  降到 S-mode 的 _entry
// 做完后 S-mode 的处境与有固件时完全一样, entry.S 一个字都不用改。
#include <platform.h>

.section .text.entry
.global _entry_m
_entry_m:
        /* ---- 1. PMP: 允许 S/U-mode 访问全部物理内存 ---- */
        li      t0, -1
        csrw    pmpaddr0, t0            /* NAPOT 覆盖整个地址空间 */
        li      t0, 0x1F                /* A=NAPOT | R | W | X */
        csrw    pmpcfg0, t0

        /* ---- 2. 把所有异常/中断委派给 S-mode ---- */
        li      t0, -1
        csrw    medeleg, t0
        csrw    mideleg, t0

        /* ---- 3. 打开 Sstc 与 counter 访问 ---- */
        li      t0, 1
        slli    t0, t0, 63              /* menvcfg.STCE = bit 63 */
        csrw    menvcfg, t0
        li      t0, -1
        csrw    mcounteren, t0          /* S-mode 可读 cycle/time/instret */

        /* ---- 4. mret 到 S-mode 入口 (保留 a0/a1) ---- */
        la      t0, _entry
        csrw    mepc, t0
        li      t0, (1 << 11)           /* mstatus.MPP = 01 -> S-mode */
        csrw    mstatus, t0
        mret
