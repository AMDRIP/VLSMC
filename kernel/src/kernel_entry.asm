[BITS 16]
section .text.entry
global _start
extern kernel_main

_start:
    cli                     ; Отключаем прерывания

    ; Загрузчик оставил нас по физическому адресу 0x10000
    xor ax, ax
    mov ds, ax
    lgdt [DWORD gdt_descriptor]

    ; Включаем Protected Mode (PE бит в CR0)
    mov eax, cr0
    or eax, 1
    mov cr0, eax

    ; Дальний прыжок (Far Jump) очистит конвейер и загрузит сегмент кода (0x08)
    jmp dword 0x08:start32

[BITS 32]
start32:
    ; Теперь мы в полноценном 32-битном режиме
    mov ax, 0x10            ; 0x10 - селектор сегмента данных
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Устанавливаем стек ядра (вершина на 0x90000)
    mov esp, 0x90000
    mov ebp, esp

    ; Вызываем глобальные конструкторы (если они есть), затем C++ функцию
    call kernel_main

    ; Защита от возврата из kernel_main
.halt:
    cli
    hlt
    jmp .halt

; ==============================================================================
; Глобальная таблица дескрипторов (GDT) для переключения в Protected Mode
; ==============================================================================
align 8
gdt_start:
    ; Нулевой дескриптор (обязателен)
    dq 0

    ; Code Segment Descriptor (Селектор 0x08)
    ; Base: 0x0, Limit: 0xFFFFF, G: 4KB pages, 32-bit, Type: Exec/Read
    dw 0xFFFF       ; Limit 0:15
    dw 0x0000       ; Base 0:15
    db 0x00         ; Base 16:23
    db 10011010b    ; Access Byte (Present, Ring0, Exec/Read)
    db 11001111b    ; Flags (4KB, 32-bit) + Limit 16:19
    db 0x00         ; Base 24:31

    ; Data Segment Descriptor (Селектор 0x10)
    ; Base: 0x0, Limit: 0xFFFFF, G: 4KB pages, 32-bit, Type: Read/Write
    dw 0xFFFF       ; Limit 0:15
    dw 0x0000       ; Base 0:15
    db 0x00         ; Base 16:23
    db 10010010b    ; Access Byte (Present, Ring0, Read/Write)
    db 11001111b    ; Flags + Limit
    db 0x00         ; Base 24:31
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1  ; Лимит GDT (Размер - 1)
    dd gdt_start                ; Физический адрес GDT базы
