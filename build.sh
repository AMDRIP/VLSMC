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
x86_64-linux-gnu-g++ $CXXFLAGS -c kernel/src/vga.cpp -o vga.o
x86_64-linux-gnu-g++ $CXXFLAGS -c kernel/src/selftest.cpp -o selftest.o
x86_64-linux-gnu-g++ $CXXFLAGS -c kernel/src/pci.cpp -o pci.o
x86_64-linux-gnu-g++ $CXXFLAGS -c kernel/src/memory_validator.cpp -o memory_validator.o
x86_64-linux-gnu-g++ $CXXFLAGS -c kernel/src/mouse.cpp -o mouse.o
x86_64-linux-gnu-g++ $CXXFLAGS -c kernel/src/bga.cpp -o bga.o
x86_64-linux-gnu-g++ $CXXFLAGS -c kernel/src/ahci.cpp -o ahci.o
x86_64-linux-gnu-g++ $CXXFLAGS -c kernel/src/disk.cpp -o disk.o

echo "[4/5] Linking kernel..."
x86_64-linux-gnu-ld -m elf_i386 -T kernel/linker.ld \
    kernel_entry.o interrupts.o switch_task.o \
    idt.o pic.o pmm.o kmalloc.o libc.o syscalls_posix.o \
    keyboard.o thread.o timer.o task_scheduler.o event_channel.o vmm.o tss.o syscall_gate.o usermode.o ata.o fat16.o elf_loader.o rtc.o pci.o memory_validator.o mouse.o bga.o ahci.o disk.o \
    shell.o shell_history.o shell_autocomplete.o shell_redirect.o vga.o selftest.o \
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

echo "=== Building Libc ==="
x86_64-linux-gnu-g++ -m32 -ffreestanding -fno-pie -fno-exceptions -fno-rtti -nostdlib -Iuser/libc/include -c user/libc/src/syscall.cpp -o user_libc_syscall.o
x86_64-linux-gnu-g++ -m32 -ffreestanding -fno-pie -fno-exceptions -fno-rtti -nostdlib -Iuser/libc/include -c user/libc/src/errno.cpp -o user_libc_errno.o
x86_64-linux-gnu-g++ -m32 -ffreestanding -fno-pie -fno-exceptions -fno-rtti -nostdlib -Iuser/libc/include -c user/libc/src/string.cpp -o user_libc_string.o
x86_64-linux-gnu-g++ -m32 -ffreestanding -fno-pie -fno-exceptions -fno-rtti -nostdlib -Iuser/libc/include -c user/libc/src/malloc.cpp -o user_libc_malloc.o
x86_64-linux-gnu-g++ -m32 -ffreestanding -fno-pie -fno-exceptions -fno-rtti -nostdlib -Iuser/libc/include -c user/libc/src/stdio.cpp -o user_libc_stdio.o
x86_64-linux-gnu-ar rcs user_libc.a user_libc_syscall.o user_libc_errno.o user_libc_string.o user_libc_malloc.o user_libc_stdio.o

echo "=== Building User Programs ==="
nasm -f elf32 user/crt0.asm -o user_crt0.o
x86_64-linux-gnu-g++ -m32 -ffreestanding -fno-pie -fno-exceptions -fno-rtti -nostdlib -Iuser -c user/hello.cpp -o user_hello.o
x86_64-linux-gnu-ld -m elf_i386 -T user/user.ld user_crt0.o user_hello.o -o HELLO.ELF
mcopy -i data.img HELLO.ELF ::/HELLO.ELF

x86_64-linux-gnu-g++ -m32 -ffreestanding -fno-pie -fno-exceptions -fno-rtti -nostdlib -Iuser -c user/vesa_test.cpp -o user_vesa_test.o
x86_64-linux-gnu-ld -m elf_i386 -T user/user.ld user_crt0.o user_vesa_test.o -o VESATEST.ELF
mcopy -i data.img VESATEST.ELF ::/VESATEST.ELF

x86_64-linux-gnu-g++ -m32 -ffreestanding -fno-pie -fno-exceptions -fno-rtti -nostdlib -Iuser -c user/fs_driver.cpp -o user_fs_driver.o
x86_64-linux-gnu-ld -m elf_i386 -T user/user.ld user_crt0.o user_fs_driver.o -o FSDRIVER.ELF
mcopy -i data.img FSDRIVER.ELF ::/FSDRIVER.ELF

x86_64-linux-gnu-g++ -m32 -ffreestanding -fno-pie -fno-exceptions -fno-rtti -nostdlib -Iuser -c user/cat_test.cpp -o user_cat_test.o
x86_64-linux-gnu-ld -m elf_i386 -T user/user.ld user_crt0.o user_cat_test.o -o CAT_TEST.ELF
mcopy -i data.img CAT_TEST.ELF ::/CAT_TEST.ELF

x86_64-linux-gnu-g++ -m32 -ffreestanding -fno-pie -fno-exceptions -fno-rtti -nostdlib -Iuser -c user/stack_bomb.cpp -o user_stack_bomb.o
x86_64-linux-gnu-ld -m elf_i386 -T user/user.ld user_crt0.o user_stack_bomb.o -o STACKBM.ELF
mcopy -i data.img STACKBM.ELF ::/STACKBM.ELF

x86_64-linux-gnu-g++ -m32 -ffreestanding -fno-pie -fno-exceptions -fno-rtti -nostdlib -nostdinc -Iuser/libc/include -c user/syscall_test.cpp -o user_syscall_test.o
x86_64-linux-gnu-ld -m elf_i386 -T user/user.ld user_crt0.o user_syscall_test.o user_libc.a -o SYSCALLT.ELF
mcopy -i data.img SYSCALLT.ELF ::/SYSCALLT.ELF

x86_64-linux-gnu-g++ -m32 -ffreestanding -fno-pie -fno-exceptions -fno-rtti -nostdlib -nostdinc -Iuser/libc/include -c user/memtest.cpp -o user_memtest.o
x86_64-linux-gnu-ld -m elf_i386 -T user/user.ld user_crt0.o user_memtest.o user_libc.a -o MEMTEST.ELF
mcopy -i data.img MEMTEST.ELF ::/MEMTEST.ELF

x86_64-linux-gnu-g++ -m32 -ffreestanding -fno-pie -fno-exceptions -fno-rtti -nostdlib -nostdinc -Iuser/libc/include -c user/malloctest.cpp -o user_malloctest.o
x86_64-linux-gnu-ld -m elf_i386 -T user/user.ld user_crt0.o user_malloctest.o user_libc.a -o MALLOCT.ELF
mcopy -i data.img MALLOCT.ELF ::/MALLOCT.ELF

x86_64-linux-gnu-g++ -m32 -ffreestanding -fno-pie -fno-exceptions -fno-rtti -nostdlib -nostdinc -Iuser/libc/include -c user/strtest.cpp -o user_strtest.o
x86_64-linux-gnu-ld -m elf_i386 -T user/user.ld user_crt0.o user_strtest.o user_libc.a -o STRTEST.ELF
mcopy -i data.img STRTEST.ELF ::/STRTEST.ELF

x86_64-linux-gnu-g++ -m32 -ffreestanding -fno-pie -fno-exceptions -fno-rtti -nostdlib -nostdinc -Iuser/libc/include -c user/stdio_test.cpp -o user_stdio_test.o
x86_64-linux-gnu-ld -m elf_i386 -T user/user.ld user_crt0.o user_stdio_test.o user_libc.a -o STDIOTST.ELF
mcopy -i data.img STDIOTST.ELF ::/STDIOTST.ELF

echo ""
echo "DONE! To run:"
echo "qemu-system-i386 -fda disk.img -hda data.img -boot a"
