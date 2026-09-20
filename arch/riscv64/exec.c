#include <kernel/elf.h>
bool arch_elf_machine(uint16 machine) { return machine == 243; }
void arch_exec_frame(trapframe_t *frame, uint64 pc, uint64 sp, uint64 argc, uint64 argv)
{
    for (unsigned i = 0; i < 32; ++i) frame->x[i] = 0;
    frame->epc = pc; frame->status = 1UL << 5;
    frame->x[2] = sp; frame->x[10] = argc; frame->x[11] = argv;
}
