OUTPUT_FORMAT(elf32-i386)

SECTIONS
{
    . = 0x00000000;

    .text : ALIGN(4K) {
        *(.text*)
    }

    .rodata : ALIGN(4K) {
        *(.rodata*)
    }

    .dynsym : {
        *(.dynsym)
    }

    .dynstr : {
        *(.dynstr)
    }

    .hash : {
        *(.hash)
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
