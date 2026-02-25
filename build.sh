#!/bin/bash
set -e

echo "=== VLSMC Bare-Metal Build Script ==="

echo "[1/5] Assembling bootloader (Stage 1 + Stage 2)..."
nasm -f bin boot/bootloader.asm -o bootloader.bin
nasm -f bin boot/stage2.asm -o STAGE2.BIN

echo "[2/5] Assembling kernel entry and interrupts..."
nasm -f elf32 kernel/src/kernel_entry.asm -o kernel_entry.o
nasm -f elf32 kernel/src/interrupts.asm -o interrupts.o
nasm -f elf32 kernel/src/switch_task.asm -o switch_task.o

echo "[3/5] Compiling C++ kernel sources..."
CXXFLAGS="-m32 -ffreestanding -fno-exceptions -fno-rtti -Ikernel/include -fpermissive -Wall -Wextra"
x86_64-linux-gnu-g++ $CXXFLAGS -c kernel/src/kernel_main.cpp -o kernel_main.o
x86_64-linux-gnu-g++ $CXXFLAGS -c kernel/src/idt.cpp -o idt.o
x86_64-linux-gnu-g++ $CXXFLAGS -c kernel/src/pic.cpp -o pic.o
x86_64-linux-gnu-g++ $CXXFLAGS -c kernel/src/pmm.cpp -o pmm.o
x86_64-linux-gnu-g++ $CXXFLAGS -c kernel/src/kmalloc.cpp -o kmalloc.o
x86_64-linux-gnu-g++ $CXXFLAGS -c kernel/src/syscalls_posix.cpp -o syscalls_posix.o
x86_64-linux-gnu-g++ $CXXFLAGS -fno-stack-protector -c kernel/src/libc.cpp -o libc.o
x86_64-linux-gnu-g++ $CXXFLAGS -c kernel/src/keyboard.cpp -o keyboard.o
x86_64-linux-gnu-g++ $CXXFLAGS -c kernel/src/thread.cpp -o thread.o
x86_64-linux-gnu-g++ $CXXFLAGS -c kernel/src/timer.cpp -o timer.o
x86_64-linux-gnu-g++ $CXXFLAGS -c kernel/src/task_scheduler.cpp -o task_scheduler.o
x86_64-linux-gnu-g++ $CXXFLAGS -c kernel/src/event_channel.cpp -o event_channel.o
x86_64-linux-gnu-g++ $CXXFLAGS -c kernel/src/vmm.cpp -o vmm.o
x86_64-linux-gnu-g++ $CXXFLAGS -c kernel/src/tss.cpp -o tss.o
x86_64-linux-gnu-g++ $CXXFLAGS -c kernel/src/syscall_gate.cpp -o syscall_gate.o
x86_64-linux-gnu-g++ $CXXFLAGS -c kernel/src/usermode.cpp -o usermode.o
x86_64-linux-gnu-g++ $CXXFLAGS -c kernel/src/ata.cpp -o ata.o
x86_64-linux-gnu-g++ $CXXFLAGS -c kernel/src/fat16.cpp -o fat16.o
x86_64-linux-gnu-g++ $CXXFLAGS -c kernel/src/elf_loader.cpp -o elf_loader.o
x86_64-linux-gnu-g++ $CXXFLAGS -c kernel/src/rtc.cpp -o rtc.o
x86_64-linux-gnu-g++ $CXXFLAGS -c kernel/src/shell.cpp -o shell.o
x86_64-linux-gnu-g++ $CXXFLAGS -c kernel/src/shell_history.cpp -o shell_history.o
x86_64-linux-gnu-g++ $CXXFLAGS -c kernel/src/shell_autocomplete.cpp -o shell_autocomplete.o
x86_64-linux-gnu-g++ $CXXFLAGS -c kernel/src/shell_redirect.cpp -o shell_redirect.o

echo "[4/5] Linking kernel..."
x86_64-linux-gnu-ld -m elf_i386 -T kernel/linker.ld \
    kernel_entry.o interrupts.o switch_task.o \
    idt.o pic.o pmm.o kmalloc.o libc.o syscalls_posix.o \
    keyboard.o thread.o timer.o task_scheduler.o event_channel.o vmm.o tss.o syscall_gate.o usermode.o ata.o fat16.o elf_loader.o rtc.o \
    shell.o shell_history.o shell_autocomplete.o shell_redirect.o \
    kernel_main.o -o kernel.elf
x86_64-linux-gnu-objcopy -O binary kernel.elf KERNEL.BIN

echo "[5/5] Building Disk Images..."

dd if=/dev/zero of=disk.img bs=512 count=2880 status=none
mkfs.fat -F 12 disk.img

dd if=bootloader.bin of=disk.img bs=1 count=3 conv=notrunc status=none
dd if=bootloader.bin of=disk.img bs=1 skip=62 seek=62 count=450 conv=notrunc status=none

mcopy -i disk.img STAGE2.BIN ::/STAGE2.BIN
mcopy -i disk.img KERNEL.BIN ::/KERNEL.BIN

echo "--- Boot disk ---"
mdir -i disk.img ::/

echo "=== Building Data Disk (FAT16) ==="
dd if=/dev/zero of=data.img bs=512 count=32768 status=none
mkfs.fat -F 16 data.img
echo "Hello from FAT16 filesystem!" | mcopy -i data.img - ::/HELLO.TXT
echo "This is a test file for the RE36 OS." | mcopy -i data.img - ::/TEST.TXT
echo "int main() { return 42; }" | mcopy -i data.img - ::/MAIN.C

echo "=== Building User Programs ==="
nasm -f elf32 user/crt0.asm -o user_crt0.o
x86_64-linux-gnu-gcc -m32 -ffreestanding -fno-pie -nostdlib -nostdinc -Iuser -c user/hello.c -o user_hello.o
x86_64-linux-gnu-ld -m elf_i386 -T user/user.ld user_crt0.o user_hello.o -o HELLO.ELF
mcopy -i data.img HELLO.ELF ::/HELLO.ELF

echo ""
echo "DONE! To run:"
echo "qemu-system-i386 -fda disk.img -hda data.img -boot a"
