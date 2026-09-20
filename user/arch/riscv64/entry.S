.section .text.entry
.global _start
_start:
    call user_main
1:  j 1b
