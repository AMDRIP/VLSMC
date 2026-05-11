[BITS 16]
[ORG 0x8000]

%define FAT_BUF  0x2000

%define BOOT_INFO_ADDR 0x0500
%define BOOT_INFO_MAGIC 0xB0071AF0
%define BOOT_INFO_VERSION 1
%define BOOT_INFO_SIZE 688
%define BOOT_INFO_MAX_MEMORY_MAP_ENTRIES 32
%define BOOT_INFO_MEMORY_MAP_ENTRY_SIZE 20
%define BOOT_INFO_FLAG_E820_VALID 0x00000001
%define BOOT_INFO_FLAG_MEMORY_FALLBACK 0x00000002
%define BOOT_INFO_FLAG_MEMORY_MAP_TRUNCATED 0x00000004
%define BOOT_MEMORY_TYPE_USABLE 1
%define KERNEL_BOUNCE_SEG 0x1000
%define KERNEL_BOUNCE_ADDR 0x00010000
%define KERNEL_RUNTIME_ADDR 0x00100000
%define KERNEL_STAGING_LIMIT 0x000A0000
%define DEFAULT_BOOT_STACK_TOP 0x00200000
%define DEFAULT_BOOT_STACK_SIZE 0x00010000

%define BI_MAGIC                 0
%define BI_VERSION               4
%define BI_SIZE                  6
%define BI_FLAGS                 8
%define BI_BOOT_DRIVE            12
%define BI_VIDEO_MODE            13
%define BI_MEMORY_MAP_COUNT      14
%define BI_MEMORY_MAP_ENTRY_SIZE 16
%define BI_CONVENTIONAL_KB       20
%define BI_KERNEL_LOAD_ADDR      24
%define BI_KERNEL_LOAD_SIZE      28
%define BI_KERNEL_ENTRY_ADDR     32
%define BI_BOOT_STACK_TOP        36
%define BI_BOOT_STACK_SIZE       40
%define BI_MEMORY_MAP            48

stage2_entry:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti

    mov [boot_drive], dl

    mov si, msg_s2
    call print

    mov si, msg_map
    call print

    xor ah, ah
    mov al, [NumberOfFATs]
    mul word [SectorsPerFAT]
    add ax, [ReservedSectors]
    mov [root_lba], ax

    mov ax, [RootEntries]
    shr ax, 4
    mov [root_sz], ax

    mov ax, [root_lba]
    add ax, [root_sz]
    mov [data_lba], ax

    mov si, msg_fat
    call print

    xor ax, ax
    mov es, ax
    mov ax, [ReservedSectors]
    mov cx, [SectorsPerFAT]
    mov bx, FAT_BUF
    call read_sectors

    mov si, msg_fok
    call print

    mov si, msg_search
    call print

    mov cx, [RootEntries]
    mov di, 0x7E00
.search:
    push cx
    mov si, kname
    mov cx, 11
    push di
    repe cmpsb
    pop di
    je .found
    add di, 32
    pop cx
    loop .search

    mov si, msg_nf
    call print
    jmp hang

.found:
    pop cx

    mov ax, [di + 26]
    mov [kernel_cluster], ax

    mov si, msg_clust
    call print
    call print_dec
    mov si, msg_crlf
    call print

    cmp ax, 2
    jb .bad_cluster
    cmp ax, 0x0FF0
    jae .bad_cluster
    jmp .cluster_ok

.bad_cluster:
    mov si, msg_bc
    call print
    jmp hang

.cluster_ok:
    mov eax, [di + 28]
    mov [kernel_file_size], eax

    mov si, msg_load
    call print

    mov eax, [kernel_file_size]
    add eax, 511
    and eax, 0xFFFFFE00
    cmp eax, KERNEL_STAGING_LIMIT - KERNEL_BOUNCE_ADDR
    ja .kernel_too_big

    mov ax, KERNEL_BOUNCE_SEG
    mov es, ax
    xor bx, bx
    xor bp, bp

.load_loop:
    mov ax, [kernel_cluster]

    cmp ax, 0x0FF8
    jae .load_done
    cmp ax, 2
    jb .chain_err
    cmp ax, 0x0FF0
    jae .chain_err

    push ax
    sub ax, 2
    xor ch, ch
    mov cl, [SectorsPerCluster]
    mul cx
    add ax, [data_lba]

    cmp ax, [TotalSectors]
    jae .lba_err

    mov cx, 1
    call read_sectors_es

    pop ax
    inc bp

    mov cx, es
    add cx, 0x0020
    mov es, cx

    call fat12_next
    mov [kernel_cluster], ax
    jmp .load_loop

.lba_err:
    pop ax
    mov si, msg_lba
    call print
    jmp hang

.chain_err:
    mov si, msg_ce
    call print
    jmp hang

.kernel_too_big:
    mov si, msg_ktb
    call print
    jmp hang

.load_done:
    xor ax, ax
    mov es, ax

    mov si, msg_sec
    call print
    mov ax, bp
    call print_dec
    mov si, msg_sectors
    call print

    mov si, msg_mem
    call print

    call init_boot_info
    call detect_legacy_memory_kb
    call detect_memory_map
    jnc .memory_ready
    call synthesize_legacy_memory_map

.memory_ready:
    mov eax, [legacy_total_kb]
    call print_dec32
    mov si, msg_kb
    call print

    cmp word [0x7DFE], 0xAA55
    jne .bad_magic

    mov si, msg_ok
    call print

    call enable_a20
    cli
    lgdt [gdt_descriptor]

    mov eax, cr0
    or eax, 1
    mov cr0, eax

    jmp 0x08:start32

.bad_magic:
    mov si, msg_mg
    call print

hang:
    cli
    hlt
    jmp hang

init_boot_info:
    xor ax, ax
    mov es, ax
    mov di, BOOT_INFO_ADDR
    mov cx, BOOT_INFO_SIZE / 2
    rep stosw

    mov dword [BOOT_INFO_ADDR + BI_MAGIC], BOOT_INFO_MAGIC
    mov word [BOOT_INFO_ADDR + BI_VERSION], BOOT_INFO_VERSION
    mov word [BOOT_INFO_ADDR + BI_SIZE], BOOT_INFO_SIZE
    mov al, [boot_drive]
    mov byte [BOOT_INFO_ADDR + BI_BOOT_DRIVE], al
    mov byte [BOOT_INFO_ADDR + BI_VIDEO_MODE], 0x03
    mov word [BOOT_INFO_ADDR + BI_MEMORY_MAP_ENTRY_SIZE], BOOT_INFO_MEMORY_MAP_ENTRY_SIZE
    mov dword [BOOT_INFO_ADDR + BI_KERNEL_LOAD_ADDR], KERNEL_RUNTIME_ADDR
    mov eax, [kernel_file_size]
    mov dword [BOOT_INFO_ADDR + BI_KERNEL_LOAD_SIZE], eax
    mov dword [BOOT_INFO_ADDR + BI_KERNEL_ENTRY_ADDR], KERNEL_RUNTIME_ADDR
    mov dword [BOOT_INFO_ADDR + BI_BOOT_STACK_TOP], DEFAULT_BOOT_STACK_TOP
    mov dword [BOOT_INFO_ADDR + BI_BOOT_STACK_SIZE], DEFAULT_BOOT_STACK_SIZE

    int 0x12
    mov word [BOOT_INFO_ADDR + BI_CONVENTIONAL_KB], ax
    ret

detect_memory_map:
    xor ax, ax
    mov es, ax
    mov di, BOOT_INFO_ADDR + BI_MEMORY_MAP
    xor ebx, ebx
    mov word [BOOT_INFO_ADDR + BI_MEMORY_MAP_COUNT], 0

.loop:
    mov eax, 0xE820
    mov edx, 0x534D4150
    mov ecx, BOOT_INFO_MEMORY_MAP_ENTRY_SIZE
    int 0x15
    jc .fail
    cmp eax, 0x534D4150
    jne .fail

    mov eax, [di + 8]
    or eax, [di + 12]
    jz .next

    inc word [BOOT_INFO_ADDR + BI_MEMORY_MAP_COUNT]
    add di, BOOT_INFO_MEMORY_MAP_ENTRY_SIZE
    cmp word [BOOT_INFO_ADDR + BI_MEMORY_MAP_COUNT], BOOT_INFO_MAX_MEMORY_MAP_ENTRIES
    jb .next
    or dword [BOOT_INFO_ADDR + BI_FLAGS], BOOT_INFO_FLAG_MEMORY_MAP_TRUNCATED
    jmp .done

.next:
    test ebx, ebx
    jnz .loop

.done:
    cmp word [BOOT_INFO_ADDR + BI_MEMORY_MAP_COUNT], 0
    je .fail
    or dword [BOOT_INFO_ADDR + BI_FLAGS], BOOT_INFO_FLAG_E820_VALID
    clc
    ret

.fail:
    mov word [BOOT_INFO_ADDR + BI_MEMORY_MAP_COUNT], 0
    stc
    ret

detect_legacy_memory_kb:
    movzx eax, word [BOOT_INFO_ADDR + BI_CONVENTIONAL_KB]
    mov [legacy_total_kb], eax

    xor eax, eax
    xor ebx, ebx
    mov ax, 0xE801
    int 0x15
    jc .try_88
    mov [legacy_mem_1m_16m_kb], ax
    mov [legacy_mem_above_16m_64kb], bx
    jmp .accumulate

.try_88:
    mov ah, 0x88
    int 0x15
    jc .done
    mov [legacy_mem_1m_16m_kb], ax
    mov word [legacy_mem_above_16m_64kb], 0

.accumulate:
    movzx eax, word [legacy_mem_1m_16m_kb]
    add [legacy_total_kb], eax
    movzx eax, word [legacy_mem_above_16m_64kb]
    shl eax, 6
    add [legacy_total_kb], eax

.done:
    ret

synthesize_legacy_memory_map:
    mov word [BOOT_INFO_ADDR + BI_MEMORY_MAP_COUNT], 0
    or dword [BOOT_INFO_ADDR + BI_FLAGS], BOOT_INFO_FLAG_MEMORY_FALLBACK

    mov ax, [BOOT_INFO_ADDR + BI_CONVENTIONAL_KB]
    test ax, ax
    jz .skip_low

    mov di, BOOT_INFO_ADDR + BI_MEMORY_MAP
    mov dword [di + 0], 0
    mov dword [di + 4], 0
    movzx eax, ax
    shl eax, 10
    mov dword [di + 8], eax
    mov dword [di + 12], 0
    mov dword [di + 16], BOOT_MEMORY_TYPE_USABLE
    inc word [BOOT_INFO_ADDR + BI_MEMORY_MAP_COUNT]

.skip_low:
    movzx eax, word [legacy_mem_1m_16m_kb]
    movzx ebx, word [legacy_mem_above_16m_64kb]
    shl ebx, 6
    add eax, ebx
    test eax, eax
    jz .done

    mov di, BOOT_INFO_ADDR + BI_MEMORY_MAP
    cmp word [BOOT_INFO_ADDR + BI_MEMORY_MAP_COUNT], 0
    je .write_ext
    add di, BOOT_INFO_MEMORY_MAP_ENTRY_SIZE

.write_ext:
    mov dword [di + 0], 0x00100000
    mov dword [di + 4], 0
    shl eax, 10
    mov dword [di + 8], eax
    mov dword [di + 12], 0
    mov dword [di + 16], BOOT_MEMORY_TYPE_USABLE
    inc word [BOOT_INFO_ADDR + BI_MEMORY_MAP_COUNT]

.done:
    ret

enable_a20:
    in al, 0x92
    test al, 2
    jnz .done
    or al, 2
    and al, 0xFE
    out 0x92, al
.done:
    ret

fat12_next:
    push bx
    push cx
    mov cx, ax
    mov bx, ax
    shr bx, 1
    add bx, ax
    mov ax, [FAT_BUF + bx]
    test cx, 1
    jz .even
    shr ax, 4
    jmp .done
.even:
    and ax, 0x0FFF
.done:
    pop cx
    pop bx
    ret

print:
    pusha
    mov ah, 0x0E
.lp:
    lodsb
    test al, al
    jz .dn
    int 0x10
    jmp .lp
.dn:
    popa
    ret

print_dec:
    pusha
    mov bx, 10
    xor cx, cx
.div_loop:
    xor dx, dx
    div bx
    push dx
    inc cx
    test ax, ax
    jnz .div_loop
.print_loop:
    pop dx
    add dl, '0'
    mov ah, 0x0E
    mov al, dl
    int 0x10
    loop .print_loop
    popa
    ret

print_dec32:
    pushad
    mov ebx, 10
    xor ecx, ecx
.div_loop:
    xor edx, edx
    div ebx
    push dx
    inc ecx
    test eax, eax
    jnz .div_loop
.print_loop:
    pop dx
    add dl, '0'
    mov ah, 0x0E
    mov al, dl
    int 0x10
    loop .print_loop
    popad
    ret

read_sectors:
    pusha
.rl:
    push ax
    push cx
    xor dx, dx
    div word [SectorsPerTrack]
    inc dl
    mov cl, dl
    xor dx, dx
    div word [HeadsPerCylinder]
    mov ch, al
    mov dh, dl
    mov ah, 0x02
    mov al, 1
    mov dl, [boot_drive]
    int 0x13
    jc .err
    pop cx
    pop ax
    add bx, 512
    inc ax
    loop .rl
    popa
    ret
.err:
    mov si, msg_de
    call print
    jmp hang

read_sectors_es:
    push ax
    push cx
    push dx
    xor dx, dx
    div word [SectorsPerTrack]
    inc dl
    mov cl, dl
    xor dx, dx
    div word [HeadsPerCylinder]
    mov ch, al
    mov dh, dl
    mov ah, 0x02
    mov al, 1
    mov dl, [boot_drive]
    int 0x13
    jc .err2
    pop dx
    pop cx
    pop ax
    ret
.err2:
    mov si, msg_de
    call print
    jmp hang

boot_drive      db 0
root_lba        dw 0
root_sz         dw 0
data_lba        dw 0
kernel_cluster  dw 0
kernel_file_size dd 0
legacy_mem_1m_16m_kb dw 0
legacy_mem_above_16m_64kb dw 0
legacy_total_kb dd 0

ReservedSectors  equ 0x7C0E
NumberOfFATs     equ 0x7C10
RootEntries      equ 0x7C11
TotalSectors     equ 0x7C13
SectorsPerFAT    equ 0x7C16
SectorsPerTrack  equ 0x7C18
HeadsPerCylinder equ 0x7C1A
SectorsPerCluster equ 0x7C0D

kname       db "KERNEL  BIN"
msg_s2      db "[Stage2] Init", 13, 10, 0
msg_map     db "[Stage2] Stack=0:7C00 FAT=0x2000", 13, 10, 0
msg_fat     db "[Stage2] Reading FAT (9 sec)...", 13, 10, 0
msg_fok     db "[Stage2] FAT loaded OK", 13, 10, 0
msg_search  db "[Stage2] Searching KERNEL.BIN...", 13, 10, 0
msg_clust   db "[Stage2] First cluster: ", 0
msg_load    db "[Stage2] Loading kernel (FAT12 chain)...", 13, 10, 0
msg_ok      db "[Stage2] Boot OK! -> 0x100000", 13, 10, 0
msg_nf      db "[Stage2] ERR: KERNEL.BIN not found!", 13, 10, 0
msg_de      db "[Stage2] ERR: Disk read!", 13, 10, 0
msg_bc      db "[Stage2] ERR: Bad cluster!", 13, 10, 0
msg_ce      db "[Stage2] ERR: Chain corrupt!", 13, 10, 0
msg_ktb     db "[Stage2] ERR: Kernel too large for staging buffer!", 13, 10, 0
msg_lba     db "[Stage2] ERR: LBA out of range!", 13, 10, 0
msg_mg      db "[Stage2] ERR: Bad boot magic!", 13, 10, 0
msg_mem     db "[Stage2] RAM: ", 0
msg_kb      db " KB", 13, 10, 0
msg_sec     db "[Stage2] Loaded ", 0
msg_sectors db " sectors", 13, 10, 0
msg_crlf    db 13, 10, 0

align 8
gdt_start:
    dq 0

    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 10011010b
    db 11001111b
    db 0x00

    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 10010010b
    db 11001111b
    db 0x00
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

[BITS 32]
start32:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    mov dword [0xB8002], 0x0E430E43
    mov ebx, BOOT_INFO_ADDR

    mov esi, KERNEL_BOUNCE_ADDR
    mov edi, [ebx + BI_KERNEL_LOAD_ADDR]
    mov ecx, [ebx + BI_KERNEL_LOAD_SIZE]
    mov edx, ecx
    shr ecx, 2
    rep movsd
    mov ecx, edx
    and ecx, 3
    rep movsb

    jmp 0x08:KERNEL_RUNTIME_ADDR
