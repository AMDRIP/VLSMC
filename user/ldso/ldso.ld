ENTRY(_start)
OUTPUT_FORMAT(elf32-i386)

SECTIONS
{
    . = SIZEOF_HEADERS;

    .text : ALIGN(16) {
        *(.text*)
    }

    .rodata : ALIGN(4) {
        *(.rodata*)
    }

    .data : ALIGN(4K) {
        *(.data*)
    }

    .dynamic : {
        *(.dynamic)
    }

    .got : {
        *(.got*)
    }

    .rel.dyn : {
        *(.rel.dyn)
        *(.rel.plt)
    }

    .bss : ALIGN(4K) {
        *(.bss*)
        *(COMMON)
    }
}
