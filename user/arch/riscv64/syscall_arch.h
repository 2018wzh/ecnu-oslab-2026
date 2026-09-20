#ifndef OSLAB_USER_SYSCALL_ARCH_H
#define OSLAB_USER_SYSCALL_ARCH_H
/* RISC-V 用户 ABI：a7 为调用号，a0～a5 为参数，a0 为返回值。 */
static inline long arch_syscall6(long number, long x0, long x1, long x2,
                                 long x3, long x4, long x5)
{
    register long a0 __asm__("a0") = x0;
    register long a1 __asm__("a1") = x1;
    register long a2 __asm__("a2") = x2;
    register long a3 __asm__("a3") = x3;
    register long a4 __asm__("a4") = x4;
    register long a5 __asm__("a5") = x5;
    register long a7 __asm__("a7") = number;
    __asm__ volatile("ecall" : "+r"(a0)
        : "r"(a1), "r"(a2), "r"(a3), "r"(a4), "r"(a5), "r"(a7) : "memory");
    return a0;
}
#endif
