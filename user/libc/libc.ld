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

    .init_array : ALIGN(4) {
        PROVIDE_HIDDEN (__init_array_start = .);
        KEEP (*(SORT_BY_INIT_PRIORITY(.init_array.*) SORT_BY_INIT_PRIORITY(.ctors.*)))
        KEEP (*(.init_array EXCLUDE_FILE (*crtbegin.o *crtbegin?.o *crtend.o *crtend?.o ) .ctors))
        PROVIDE_HIDDEN (__init_array_end = .);
    }

    .fini_array : ALIGN(4) {
        PROVIDE_HIDDEN (__fini_array_start = .);
        KEEP (*(SORT_BY_INIT_PRIORITY(.fini_array.*) SORT_BY_INIT_PRIORITY(.dtors.*)))
        KEEP (*(.fini_array EXCLUDE_FILE (*crtbegin.o *crtbegin?.o *crtend.o *crtend?.o ) .dtors))
        PROVIDE_HIDDEN (__fini_array_end = .);
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
