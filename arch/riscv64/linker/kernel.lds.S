#include <platform.h>
#include <asm/boot.h>
OUTPUT_ARCH(riscv)
ENTRY(_entry)
SECTIONS {
    . = LOAD;
    .text : { KEEP(*(.text.entry)) *(.text .text.*) }
    .trampoline : ALIGN(4096) { KEEP(*(.trampoline)) }
    ASSERT(SIZEOF(.trampoline) <= 4096, "trampoline exceeds page")
    . = ALIGN(4096); text_end = .;
    .rodata : ALIGN(4096) { *(.rodata .rodata.*) }
    . = ALIGN(4096); rodata_end = .;
    .data : ALIGN(4096) { PROVIDE(__global_pointer$ = . + 0x800); *(.sdata .sdata.* .data .data.*) }
    . = ALIGN(8);
    __bss_start = .;
    .bss : { *(.sbss .sbss.* .bss .bss.*) *(COMMON) }
    . = ALIGN(8);
    __bss_end = .;
    .stacks (NOLOAD) : ALIGN(16) { boot_stacks = .; . += NCPU * STACK_SIZE; }
    . = ALIGN(4096);
    kernel_end = .;
    ASSERT((boot_stacks % 16) == 0, "stack alignment")
    /DISCARD/ : { *(.eh_frame .comment) }
}
