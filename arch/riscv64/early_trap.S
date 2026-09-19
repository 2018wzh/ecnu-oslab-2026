.section .text
.balign 4
.globl early_trap
early_trap:
    csrci sstatus, 2
1:  wfi
    j 1b
