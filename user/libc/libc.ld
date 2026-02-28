OUTPUT_FORMAT(elf32-i386)

SECTIONS
{
    . = SIZEOF_HEADERS;

    .text : ALIGN(4K) {
        *(.text*)
    }

    .rodata : ALIGN(4) {
        *(.rodata*)
    }

    .data.rel.ro : ALIGN(4) {
        *(.data.rel.ro*)
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

    .gnu.hash : {
        *(.gnu.hash)
    }

    .rel.dyn : {
        *(.rel.dyn)
        *(.rel.plt)
    }

    .dynamic : {
        *(.dynamic)
    }

    .got : ALIGN(4) {
        *(.got)
    }

    .got.plt : ALIGN(4) {
        *(.got.plt)
    }

    .data : ALIGN(4K) {
        *(.data*)
    }

    .bss : ALIGN(4K) {
        *(.bss*)
        *(COMMON)
    }

    /DISCARD/ : {
        *(.comment)
        *(.note*)
        *(.eh_frame*)
    }
}
