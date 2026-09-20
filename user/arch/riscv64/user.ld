OUTPUT_ARCH(riscv)
ENTRY(_start)
SECTIONS {
    . = 0x1000;
    .text : { *(.text.entry) *(.text .text.*) }
    .rodata : { *(.rodata .rodata.* .srodata .srodata.*) }
    .data : { *(.data .data.* .sdata .sdata.*) }
    .bss : { *(.bss .bss.* .sbss .sbss.* COMMON) }
    ASSERT(. <= 0x2000, "initcode exceeds one page")
    /DISCARD/ : { *(.eh_frame .comment) }
}
