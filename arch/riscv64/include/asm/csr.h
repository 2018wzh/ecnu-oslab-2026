#ifndef OSLAB_CSR_H
#define OSLAB_CSR_H
#include <kernel/types.h>
#define csr_read(reg) ({ uint64 x; __asm__ volatile("csrr %0, " #reg : "=r"(x)); x; })
#define csr_write(reg, x) __asm__ volatile("csrw " #reg ", %0" :: "r"((uint64)(x)) : "memory")
#define SSTATUS_SIE (1UL << 1)
#endif
