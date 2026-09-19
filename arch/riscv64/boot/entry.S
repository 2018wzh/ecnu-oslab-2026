#include <platform.h>
#include <asm/boot.h>
.section .text.entry
.globl _entry, secondary_entry
_entry:
    csrci sstatus, 2
    csrw sie, zero
    li t0, HART_FIRST
    bltu a0, t0, .Lpark
    li t1, HART_FIRST + NCPU
    bgeu a0, t1, .Lpark
    # 冷启动只进入一次；HSM 从核使用另一个入口。
    la t0, __bss_start
    la t1, __bss_end
1:  bgeu t0, t1, 2f
    sd zero, 0(t0)
    addi t0, t0, 8
    j 1b
2:  la t0, boot_hart
    sd a0, 0(t0)
    j .Lstack
secondary_entry:
    csrci sstatus, 2
    csrw sie, zero
    li t0, HART_FIRST
    bltu a0, t0, .Lpark
    li t1, HART_FIRST + NCPU
    bgeu a0, t1, .Lpark
.Lstack:
    li t0, HART_FIRST
    sub t0, a0, t0
    addi t0, t0, 1
    slli t0, t0, STACK_SHIFT
    la sp, boot_stacks
    add sp, sp, t0
    mv tp, a0
    .option push
    .option norelax
    la gp, __global_pointer$
    .option pop
    call start
.Lpark:
    wfi
    j .Lpark
