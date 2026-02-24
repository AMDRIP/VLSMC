#!/bin/bash
set -e

echo "=== VLSMC Bare-Metal Build Script ==="

# 1. Сборка загрузчика
echo "[1/4] Assembling bootloader..."
nasm -f bin boot/bootloader.asm -o bootloader.bin

# 2. Сборка входной точки ядра и прерываний (Assembly)
echo "[2/4] Assembling kernel entry and interrupts..."
nasm -f elf32 kernel/src/kernel_entry.asm -o kernel_entry.o
nasm -f elf32 kernel/src/interrupts.asm -o interrupts.o
nasm -f elf32 kernel/src/switch_task.asm -o switch_task.o

# 3. Компиляция ядра (C++)
# Флаги freestanding отключают стандартную библиотеку C/C++
echo "[3/4] Compiling C++ kernel sources..."
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

# 4. Компоновка (Link) и извлечение плоского бинарника
echo "[4/4] Linking kernel..."
x86_64-linux-gnu-ld -m elf_i386 -T kernel/linker.ld \
    kernel_entry.o interrupts.o switch_task.o \
    idt.o pic.o pmm.o kmalloc.o libc.o syscalls_posix.o \
    keyboard.o thread.o timer.o task_scheduler.o event_channel.o \
    kernel_main.o -o kernel.elf
x86_64-linux-gnu-objcopy -O binary kernel.elf KERNEL.BIN

echo "=== Building Disk Image ==="
# Создаем пустой образ дискеты 1.44MB
dd if=/dev/zero of=disk.img bs=512 count=2880 status=none

# Форматируем как FAT12 (стандарт для 1.44MB дискет)
mkfs.fat -F 12 disk.img      

# Теперь нам нужно перенести наш код загрузчика в boot sector образа disk.img,
# при этом сохранив FAT BPB (который сгенерировал mkfs.fat), чтобы файловая система осталась целой.
# Байты 0-2 (JMP и NOP)
dd if=bootloader.bin of=disk.img bs=1 count=3 conv=notrunc status=none
# Байты 62-511 (Boot Code и Сигнатура 0xAA55)
dd if=bootloader.bin of=disk.img bs=1 skip=62 seek=62 count=450 conv=notrunc status=none

# Копируем само ядро на файловую систему (чтобы загрузчик мог найти KERNEL.BIN)
mcopy -i disk.img KERNEL.BIN ::/KERNEL.BIN

echo "DONE! Generated disk.img. To test across QEMU:"
echo "qemu-system-i386 -drive format=raw,file=disk.img"
