/* ELF 可执行文件格式定义: 内核解析用户程序的 ELF, 只关心程序头表 (段), 不关心 section。 */
#ifndef __KERNEL_ELF_H__
#define __KERNEL_ELF_H__

#include <kernel/types.h>

#define ELF_MAGIC 0x464C457F   /* "\x7fELF" 小端读法 */

// ELF 头部 (64 位)。
typedef struct elf_header {
	uint8  ident[16];      /* 魔数 + 类型 + 机器等 */
	uint16 type;           /* 2 = ET_EXEC (可执行) */
	uint16 machine;        /* 243 = EM_RISCV */
	uint32 version;
	uint64 entry;          /* 程序入口地址 (虚拟地址) */
	uint64 phoff;          /* 程序头表在文件中的偏移 */
	uint64 shoff;
	uint32 flags;
	uint16 ehsize;
	uint16 phentsize;      /* 每个程序头的大小 */
	uint16 phnum;          /* 程序头数量 */
	uint16 shentsize;
	uint16 shnum;
	uint16 shstrndx;
} elf_header_t;

// 程序头。
typedef struct elf_program_header {
	uint32 type;           /* 1 = PT_LOAD (需要加载) */
	uint32 flags;          /* 1=X 2=W 4=R */
	uint64 offset;         /* 该段在文件中的偏移 */
	uint64 vaddr;          /* 应该被加载到的虚拟地址 */
	uint64 paddr;
	uint64 filesz;         /* 该段在文件中的大小 */
	uint64 memsz;          /* 该段在内存中的大小 (memsz >= filesz) */
	uint64 align;
} elf_phdr_t;

#define ELF_PT_LOAD 1
#define ELF_PF_X 1
#define ELF_PF_W 2
#define ELF_PF_R 4

// e_machine 的期望值由 arch 层提供 (见 <kernel/arch.h> 的 ARCH_ELF_MACHINE), 不写死在 generic 头里。
// 由 mk/build.mk 通过 xxd 生成的 initcode 二进制数组, 内核把用户程序嵌入自己的镜像。
// 注意是 unsigned char[] (xxd 默认不加 const), 内核只读它, 不影响正确性。
extern unsigned char initcode[];
extern unsigned int  initcode_len;

#endif /* __KERNEL_ELF_H__ */
