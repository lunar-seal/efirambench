ENTRY(stage2_start)
OUTPUT_FORMAT(elf64-x86-64)

SECTIONS {
    . = 0x8000;

    .text : {
        KEEP(*(.text.boot))
        *(.text*)
    }

    .rodata : ALIGN(8) {
        *(.rodata*)
    }

    .data : ALIGN(8) {
        *(.data*)
    }

    .bss : ALIGN(8) {
        *(.bss*)
        *(COMMON)
    }

    /DISCARD/ : {
        *(.comment)
        *(.note*)
        *(.eh_frame)
        *(.rela.*)
    }
}
