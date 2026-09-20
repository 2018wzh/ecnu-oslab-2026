#ifndef OSLAB_ELF_H
#define OSLAB_ELF_H
#include <kernel/inode.h>
#include <kernel/proc.h>
typedef struct { uint64 entry, phoff; uint16 phnum; } elf_info_t;
/* 教师辅助；调用者持有 ELF inode 锁，新页表未发布且独占。 */
int elf_read_header(inode_t *ip, elf_info_t *out);
void load_segment(inode_t *ip, pgtbl_t root, uint32 offset, uint64 va, uint32 len);
uint64 prepare_heap(pgtbl_t root, inode_t *ip, const elf_info_t *elf); /* 失败 UINT64_MAX */
int prepare_stack(pgtbl_t root, const char *const argv[], size_t argc, uint64 *sp); /* sp 也是 argv 地址 */
bool arch_elf_machine(uint16 machine);
void arch_exec_frame(trapframe_t *frame, uint64 pc, uint64 sp, uint64 argc, uint64 argv);
#endif
