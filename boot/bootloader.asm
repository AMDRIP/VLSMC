; ==============================================================================
; RAND Elecorner 36 - Real-Mode Bootloader (FAT16 Support)
; ==============================================================================
; Этот загрузчик форматируется как часть FAT16 дискеты/раздела.
; Он содержит BIOS Parameter Block (BPB), читает корневой каталог (Root Dir),
; ищет файл KERNEL.BIN и считывает его кластеры в память (0x10000).
;
; Ограничения для простоты: ядро должно быть загружено в непрерывные кластеры
; (без фрагментации), иначе код парсинга FAT будет сильно сложнее для 512 байт.
; ==============================================================================

[BITS 16]
[ORG 0x7C00]

jmp short start
nop

; ------------------------------------------------------------------------------
; BIOS Parameter Block (BPB) для FAT16
; ------------------------------------------------------------------------------
OEMName             db "RANDOS  "   ; 8 байт
BytesPerSector      dw 512          ; 512 байт на сектор
SectorsPerCluster   db 1            ; 1 сектор на кластер
ReservedSectors     dw 1            ; 1 зарезервированный сектор (наш бутсектор)
NumberOfFATs        db 2            ; 2 копии таблицы FAT
RootEntries         dw 224          ; Максимум 224 файла в корневом каталоге
TotalSectors        dw 2880         ; 1.44MB дискета
MediaDescriptor     db 0xF0         ; 0xF0 = 1.44MB Floppy
SectorsPerFAT       dw 9            ; 9 секторов на одну FAT таблицу
SectorsPerTrack     dw 18           ; 18 секторов на дорожку
HeadsPerCylinder    dw 2            ; 2 головки
HiddenSectors       dd 0            ; 0 скрытых секторов
TotalSectorsBig     dd 0            ; Не используется (TotalSectors < 65535)

; Расширенный BPB (FAT12/FAT16)
DriveNumber         db 0            ; Подставляется BIOS
Reserved            db 0            ; Reserved
Signature           db 0x29         ; Boot signature
VolumeID            dd 0x12345678   ; Volume ID
VolumeLabel         db "RAND OS VOL" ; 11 байт
FileSystem          db "FAT16   "   ; 8 байт

; ------------------------------------------------------------------------------
; Код загрузчика
; ------------------------------------------------------------------------------
start:
    ; Настройка сегментов
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti

    ; Запоминаем загрузочный диск
    mov [DriveNumber], dl

    ; Приветствие
    mov ah, 0x00
    mov al, 0x03
    int 0x10        ; Очистка экрана

    mov si, msg_welcome
    call print_string

    ; Вычисление LBA корневого каталога (Root Directory)
    ; LBA = ReservedSectors + (NumberOfFATs * SectorsPerFAT)
    mov al, [NumberOfFATs]
    xor ah, ah
    mul word [SectorsPerFAT]
    add ax, [ReservedSectors]
    mov [RootDirLBA], ax

    ; Вычисление размера Root Directory в секторах: 
    ; (RootEntries * 32) / 512
    mov ax, [RootEntries]
    shl ax, 5               ; умножить на 32 байта (размер записи)
    
    ; делим на 512 (сдвиг вправо на 9 бит)
    shr ax, 9               
    mov [RootDirSize], ax

    ; --------------------------------------------------------------------------
    ; Читаем Root Directory в память (0x7E00 = безопасно после бутсектора)
    ; --------------------------------------------------------------------------
    mov ax, [RootDirLBA]    ; Стартовый сектор
    mov cx, [RootDirSize]   ; Сколько секторов читать
    mov bx, 0x7E00          ; Буфер для чтения (безопасное место)
    call read_sectors

    ; --------------------------------------------------------------------------
    ; Ищем файл KERNEL.BIN в Root Directory
    ; --------------------------------------------------------------------------
    mov cx, [RootEntries]
    mov di, 0x7E00          ; Начало Root Directory
.search_loop:
    push cx
    mov cx, 11              ; Длина имени файла (8+3)
    mov si, kernel_filename ; Указатель на искомое имя
    push di                 ; Сохраняем текущий адрес записи
    repe cmpsb              ; Сравниваем CX байт SI и DI
    pop di
    je .found_file          ; Если совпали все 11 байт, мы нашли ядро!

    add di, 32              ; Иначе переходим к следующей записи (32 байта)
    pop cx
    loop .search_loop

    ; Если не нашли:
    jmp error_not_found

.found_file:
    ; Нашли файл! Извлекаем начальный кластер
    mov ax, [di + 26]       ; Смещение 26 = номер первого кластера (First Cluster Number)
    mov [KernelCluster], ax

    ; Вычисление LBA сектора данных (Data Region)
    ; DataRegionLBA = RootDirLBA + RootDirSize
    mov ax, [RootDirLBA]
    add ax, [RootDirSize]
    mov [DataRegionLBA], ax

    ; --------------------------------------------------------------------------
    ; Читаем ядро из сектора данных в 0x1000:0x0000 (упрощенно: считаем что оно непрерывно)
    ; Улучшенная версия должна читать FAT и прыгать по цепочке кластеров.
    ; Здесь: читаем 15 секторов (7.5 KB) начиная с First Cluster
    ; --------------------------------------------------------------------------
    mov si, msg_loading
    call print_string

    ; LBA кластера = DataRegionLBA + (Cluster - 2) * SectorsPerCluster
    mov ax, [KernelCluster]
    sub ax, 2
    xor cx, cx
    mov cl, [SectorsPerCluster]
    mul cx                  ; Теперь умножаем 16-битный AX на CX
    add ax, [DataRegionLBA]

    ; Читаем 64 сектора загруженного кластера (32 KB - УПРОЩЕНИЕ!)
    mov cx, 64
    mov bx, 0x1000          ; ES = 0x1000
    mov es, bx
    mov bx, 0x0000          ; Offset 0
    call read_sectors

    ; Возвращаем ES = 0
    xor ax, ax
    mov es, ax

    mov si, msg_success
    call print_string

    ; Прыжок в ядро!
    jmp 0x1000:0x0000

error_not_found:
    mov si, msg_missing
    call print_string
    cli
    hlt

; ==============================================================================
; Подпрограммы
; ==============================================================================

; --- Печать строки ---
print_string:
    pusha
    mov ah, 0x0E
.loop:
    lodsb
    test al, al
    jz .done
    int 0x10
    jmp .loop
.done:
    popa
    ret

; --- Чтение секторов (LBA) ---
; Вход: AX = LBA начального сектора, CX = сколько читать, ES:BX = буфер
read_sectors:
    pusha
.read_loop:
    push ax
    push cx
    push bx
    
    ; Подготавливаем Disk Address Packet (DAP) в памяти
    mov word [dap_size], 16         ; Размер структуры (16 байт)
    mov word [dap_count], 1         ; Читать 1 сектор за раз
    mov [dap_buffer_offset], bx     ; Смещение буфера (BX)
    
    mov cx, es
    mov [dap_buffer_segment], cx    ; Сегмент буфера (ES)
    
    mov [dap_lba_lower], ax         ; LBA (Младшие 32 бита, старшие 16 обнуляем)
    mov word [dap_lba_lower+2], 0
    mov dword [dap_lba_upper], 0    ; Старшие 32 бита обнуляем

    mov ah, 0x42                    ; Extended Read
    mov dl, [DriveNumber]           ; Номер диска
    mov si, dap_size                ; Указатель на DAP
    int 0x13
    jc disk_error                   ; Ошибка чтения

    ; Продвигаем буфер и LBA
    pop bx
    add bx, 512
    pop cx
    pop ax
    inc ax

    loop .read_loop

    popa
    ret

disk_error:
    mov si, msg_error
    call print_string
    cli
    hlt

; ==============================================================================
; Данные
; ==============================================================================

RootDirLBA    dw 0
RootDirSize   dw 0
DataRegionLBA dw 0
KernelCluster dw 0

; Disk Address Packet (DAP) для LBA чтения
dap_size           db 0x10 ; 16 байт
dap_reserved       db 0
dap_count          dw 0
dap_buffer_offset  dw 0
dap_buffer_segment dw 0
dap_lba_lower      dd 0
dap_lba_upper      dd 0

kernel_filename db "KERNEL  BIN" ; 11 байт (8+3), пробелы обязательны

msg_welcome db "RANDOS Boot...", 13, 10, 0
msg_loading db "Loading KERNEL.BIN...", 13, 10, 0
msg_success db "Boot OK! Jumping to 0x10000...", 13, 10, 0
msg_missing db "Err: KERNEL.BIN not found!", 13, 10, 0
msg_error   db "Err: Disk read fail!", 13, 10, 0

; ==============================================================================
; Boot Signature
; ==============================================================================
times 510-($-$$) db 0
dw 0xAA55
