[BITS 32]

extern isr_handler

; Функция загрузки IDT
global load_idt
load_idt:
    mov eax, [esp + 4]  ; Указатель на структуру idt_ptr
    lidt [eax]          ; Загружаем таблицу дескрипторов
    ret

; Макрос для прерываний без кода ошибки
%macro ISR_NOERRCODE 1
global isr%1
isr%1:
    cli
    push byte 0         ; Фейковый код ошибки
    push byte %1        ; Номер прерывания
    jmp isr_common_stub
%endmacro

; Макрос для прерываний с аппаратным кодом ошибки (например Page Fault 14)
%macro ISR_ERRCODE 1
global isr%1
isr%1:
    cli
    ; Процессор УЖЕ положил код ошибки на стек
    push byte %1        ; Номер прерывания
    jmp isr_common_stub
%endmacro

; Генерируем 32 обработчика исключений процессора
ISR_NOERRCODE 0
ISR_NOERRCODE 1
ISR_NOERRCODE 2
ISR_NOERRCODE 3
ISR_NOERRCODE 4
ISR_NOERRCODE 5
ISR_NOERRCODE 6
ISR_NOERRCODE 7
ISR_ERRCODE   8
ISR_NOERRCODE 9
ISR_ERRCODE   10
ISR_ERRCODE   11
ISR_ERRCODE   12
ISR_ERRCODE   13
ISR_ERRCODE   14
ISR_NOERRCODE 15
ISR_NOERRCODE 16
ISR_NOERRCODE 17
ISR_NOERRCODE 18
ISR_NOERRCODE 19
ISR_NOERRCODE 20
ISR_NOERRCODE 21
ISR_NOERRCODE 22
ISR_NOERRCODE 23
ISR_NOERRCODE 24
ISR_NOERRCODE 25
ISR_NOERRCODE 26
ISR_NOERRCODE 27
ISR_NOERRCODE 28
ISR_NOERRCODE 29
ISR_NOERRCODE 30
ISR_NOERRCODE 31

; IRQs (Аппаратные прерывания от PIC)
; Мы перемапим PIC так, что IRQ0-15 будут вызывать IDT 32-47
%macro IRQ_STUB 2
global irq%1
irq%1:
    cli
    push byte 0         ; Фейковый код ошибки
    push byte %2        ; Номер прерывания (32+)
    jmp isr_common_stub
%endmacro

IRQ_STUB 0, 32
IRQ_STUB 1, 33
IRQ_STUB 2, 34
IRQ_STUB 3, 35
IRQ_STUB 4, 36
IRQ_STUB 5, 37
IRQ_STUB 6, 38
IRQ_STUB 7, 39
IRQ_STUB 8, 40
IRQ_STUB 9, 41
IRQ_STUB 10, 42
IRQ_STUB 11, 43
IRQ_STUB 12, 44
IRQ_STUB 13, 45
IRQ_STUB 14, 46
IRQ_STUB 15, 47

; Syscall Gate (int 0x80 = ISR 128)
global isr128
isr128:
    cli
    push byte 0         ; Фейковый код ошибки
    push dword 128      ; Номер прерывания
    jmp isr_common_stub

; Общий кусок кода для всех прерываний
isr_common_stub:
    pusha               ; Сохраняет EDI, ESI, EBP, ESP, EBX, EDX, ECX, EAX

    mov ax, ds          ; Сохраняем Data Segment
    push eax            ; И кладем его на стек тоже (структура Registers)

    mov ax, 0x10        ; Загружаем Kernel Data Segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp            ; Передаем указатель на структуру Registers в аргумент (esp) функции
    call isr_handler    ; Вызываем глобальный C++ обработчик
    add esp, 4          ; Очищаем аргумент со стека

    pop eax             ; Восстанавливаем оригинальный Data Segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    popa                ; Восстанавливаем все регистры
    add esp, 8          ; Убираем код ошибки и номер прерывания со стека
    sti                 ; Включаем прерывания
    iret                ; Возвращаемся из прерывания из Ring 0 в прерванный код
