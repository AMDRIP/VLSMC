[BITS 16]
[ORG 0x7C00]

jmp short start
nop

OEMName             db "RANDOS  "
BytesPerSector      dw 512
SectorsPerCluster   db 1
ReservedSectors     dw 1
NumberOfFATs        db 2
RootEntries         dw 224
TotalSectors        dw 2880
MediaDescriptor     db 0xF0
SectorsPerFAT       dw 9
SectorsPerTrack     dw 18
HeadsPerCylinder    dw 2
HiddenSectors       dd 0
TotalSectorsBig     dd 0
DriveNumber         db 0
Reserved1           db 0
BootSig             db 0x29
VolumeID            dd 0x12345678
VolumeLabel         db "RAND OS VOL"
FileSystem          db "FAT12   "

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti

    mov [DriveNumber], dl

    mov ax, 0x0003
    int 0x10

    mov si, m1
    call pr

    xor ah, ah
    mov al, [NumberOfFATs]
    mul word [SectorsPerFAT]
    add ax, [ReservedSectors]
    mov [rlba], ax

    mov ax, [RootEntries]
    shr ax, 4
    mov [rsz], ax

    mov ax, [rlba]
    mov cx, [rsz]
    mov bx, 0x7E00
    call rdsec

    mov cx, [RootEntries]
    mov di, 0x7E00
.sch:
    push cx
    mov si, s2name
    mov cx, 11
    push di
    repe cmpsb
    pop di
    je .fnd
    add di, 32
    pop cx
    loop .sch
    mov si, m3
    call pr
    jmp hng

.fnd:
    pop cx
    mov ax, [di + 26]

    sub ax, 2
    add ax, [rlba]
    add ax, [rsz]

    mov cx, 8
    mov bx, 0x8000
    call rdsec

    mov si, m2
    call pr
    mov dl, [DriveNumber]
    jmp 0x0000:0x8000

hng:
    cli
    hlt
    jmp hng

pr:
    pusha
    mov ah, 0x0E
.l: lodsb
    test al, al
    jz .d
    int 0x10
    jmp .l
.d: popa
    ret

rdsec:
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
    mov dl, [DriveNumber]
    int 0x13
    jc .er
    pop cx
    pop ax
    add bx, 512
    inc ax
    loop .rl
    popa
    ret
.er:
    mov si, m4
    call pr
    jmp hng

rlba  dw 0
rsz   dw 0

s2name db "STAGE2  BIN"
m1     db "S1.", 0
m2     db "S2.", 0
m3     db "No STAGE2!", 0
m4     db "DskErr", 0

times 510-($-$$) db 0
dw 0xAA55
