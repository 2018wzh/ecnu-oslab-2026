/* "怎么发起一次系统调用" (架构相关): 调用号在 ABI 里 (架构无关),
 * 用哪条指令、号放哪个寄存器是架构事实。换架构只需本目录写同名的头。
 * 用 register...asm("a0") 寄存器变量, 让编译器把参数分配到 a0-a2、号到 a7。 */
#ifndef __USER_ARCH_RISCV64_SYSCALL_ARCH_H__
#define __USER_ARCH_RISCV64_SYSCALL_ARCH_H__

// 无参数。
static inline long __syscall0(long n)
{
	register long a7 __asm__("a7") = n;
	register long a0 __asm__("a0");

	__asm__ __volatile__("ecall" : "+r"(a0) : "r"(a7) : "memory");
	return a0;
}

// 1 个参数。
static inline long __syscall1(long n, long a)
{
	register long a7 __asm__("a7") = n;
	register long a0 __asm__("a0") = a;

	__asm__ __volatile__("ecall" : "+r"(a0) : "r"(a7) : "memory");
	return a0;
}

// 2 个参数。
static inline long __syscall2(long n, long a, long b)
{
	register long a7 __asm__("a7") = n;
	register long a0 __asm__("a0") = a;
	register long a1 __asm__("a1") = b;

	__asm__ __volatile__("ecall"
	                     : "+r"(a0)
	                     : "r"(a7), "r"(a1)
	                     : "memory");
	return a0;
}

// 3 个参数。
static inline long __syscall3(long n, long a, long b, long c)
{
	register long a7 __asm__("a7") = n;
	register long a0 __asm__("a0") = a;
	register long a1 __asm__("a1") = b;
	register long a2 __asm__("a2") = c;

	__asm__ __volatile__("ecall"
	                     : "+r"(a0)
	                     : "r"(a7), "r"(a1), "r"(a2)
	                     : "memory");
	return a0;
}

#endif /* __USER_ARCH_RISCV64_SYSCALL_ARCH_H__ */
