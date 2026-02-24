#!/bin/bash
set -e

echo "=== VLSMC Bare-Metal Build Script ==="

# 1. Сборка загрузчика
echo "[1/4] Assembling bootloader..."
nasm -f bin boot/bootloader.asm -o bootloader.bin

# 2. Сборка входной точки ядра (Assembly)
echo "[2/4] Assembling kernel entry..."
nasm -f elf32 kernel/src/kernel_entry.asm -o kernel_entry.o

# 3. Компиляция ядра (C++)
# Флаги freestanding отключают стандартную библиотеку C/C++
echo "[3/4] Compiling kernel_main.cpp..."
x86_64-linux-gnu-g++ -m32 -ffreestanding -fno-exceptions -fno-rtti -Wall -Wextra -c kernel/src/kernel_main.cpp -o kernel_main.o

# 4. Компоновка (Link) и извлечение плоского бинарника
echo "[4/4] Linking kernel..."
x86_64-linux-gnu-ld -m elf_i386 -T kernel/linker.ld kernel_entry.o kernel_main.o -o kernel.elf
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
