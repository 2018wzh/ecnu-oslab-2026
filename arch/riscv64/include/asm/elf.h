// ELF 里与架构有关的常量 (架构相关)。
// e_machine 字段说明可执行文件为哪种机器编译, 是 ISA 事实 (RISC-V=243,
// AArch64=183, x86-64=62)。通用的 ELF 加载器只问"是不是本架构", 不关心数字,
// 所以判定常量放这里加载器只写 ARCH_ELF_MACHINE。
#ifndef __ASM_ELF_H__
#define __ASM_ELF_H__

// ELF e_machine: EM_RISCV = 243 (见 docs/abi-spec.md)。
#define ARCH_ELF_MACHINE 243

#endif /* __ASM_ELF_H__ */
