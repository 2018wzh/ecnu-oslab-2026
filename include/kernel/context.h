#ifndef OSLAB_CONTEXT_H
#define OSLAB_CONTEXT_H
#include <kernel/types.h>
typedef struct { uint64 ra, sp, saved[12]; } context_t;
void arch_switch(context_t *old, const context_t *next);
/* ra/sp/s0..s11 共 14 个槽；教师汇编只切换同特权级执行流。 */
_Static_assert(sizeof(context_t) == 112, "context layout");
_Static_assert(offsetof(context_t, saved) == 16, "saved registers layout");
#endif
