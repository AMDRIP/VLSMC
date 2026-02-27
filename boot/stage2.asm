[BITS 16]
[ORG 0x8000]

%define FAT_BUF  0x2000

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
    mov si, msg_load
    call print

    mov bx, 0x1000
    mov es, bx
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

    mov dl, [boot_drive]
    mov [0x0500], dl
    mov byte [0x0501], 0x03

    xor cx, cx
    xor dx, dx
    mov ax, 0xE801
    int 0x15
    jc .try_88
    mov [0x0502], ax
    mov [0x0504], bx
    jmp .mem_ok

.try_88:
    mov ah, 0x88
    int 0x15
    mov [0x0502], ax
    mov word [0x0504], 0

.mem_ok:
    mov dword [0x0506], 0xB0071AF0

    mov ax, [0x0502]
    add ax, 1024
    call print_dec
    mov si, msg_kb
    call print

    cmp word [0x7DFE], 0xAA55
    jne .bad_magic

    mov si, msg_ok
    call print

    ; Включаем A20 Line (Fast A20) перед переходом в защищенный режим
    in al, 0x92
    test al, 2
    jnz .a20_on
    or al, 2
    and al, 0xFE
    out 0x92, al
.a20_on:

    jmp 0x1000:0x0000

.bad_magic:
    mov si, msg_mg
    call print

hang:
    cli
    hlt
    jmp hang

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
msg_ok      db "[Stage2] Boot OK! -> 0x10000", 13, 10, 0
msg_nf      db "[Stage2] ERR: KERNEL.BIN not found!", 13, 10, 0
msg_de      db "[Stage2] ERR: Disk read!", 13, 10, 0
msg_bc      db "[Stage2] ERR: Bad cluster!", 13, 10, 0
msg_ce      db "[Stage2] ERR: Chain corrupt!", 13, 10, 0
msg_lba     db "[Stage2] ERR: LBA out of range!", 13, 10, 0
msg_mg      db "[Stage2] ERR: Bad boot magic!", 13, 10, 0
msg_mem     db "[Stage2] RAM: ", 0
msg_kb      db " KB", 13, 10, 0
msg_sec     db "[Stage2] Loaded ", 0
msg_sectors db " sectors", 13, 10, 0
msg_crlf    db 13, 10, 0
