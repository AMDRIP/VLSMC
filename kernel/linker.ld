ENTRY(_start)

OUTPUT_FORMAT(elf32-i386)

SECTIONS
{
    /* Загрузчик переходит по адресу 0x10000 */
    . = 0x10000;

    _kernel_start = .;

    .text : ALIGN(4K)
    {
        *(.text.entry)
        *(.text*)
    }

    .rodata : ALIGN(4K)
    {
        *(.rodata)
        *(.rodata*)
    }

    .data : ALIGN(4K)
    {
        *(.data)
    }

    .bss : ALIGN(4K)
    {
        _bss_start = .;
        *(COMMON)
        *(.bss)
        . = ALIGN(4);
        _bss_end = .;
    }

    . = ALIGN(4K);
    _kernel_end = .;
}
