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
CXXFLAGS="-m32 -ffreestanding -fno-exceptions -fno-rtti -Ikernel/include -fpermissive -Wall -Wextra -mno-sse -mno-sse2 -mno-mmx"
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
x86_64-linux-gnu-g++ $CXXFLAGS -c kernel/src/cow.cpp -o cow.o
x86_64-linux-gnu-g++ $CXXFLAGS -c kernel/src/tss.cpp -o tss.o
x86_64-linux-gnu-g++ $CXXFLAGS -c kernel/src/syscall_gate.cpp -o syscall_gate.o
x86_64-linux-gnu-g++ $CXXFLAGS -c kernel/src/usermode.cpp -o usermode.o
x86_64-linux-gnu-g++ $CXXFLAGS -c kernel/src/ata.cpp -o ata.o
x86_64-linux-gnu-g++ $CXXFLAGS -c kernel/src/fat16.cpp -o fat16.o
x86_64-linux-gnu-g++ $CXXFLAGS -c kernel/src/vfs.cpp -o vfs.o
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
x86_64-linux-gnu-g++ $CXXFLAGS -c kernel/src/page_cache.cpp -o page_cache.o

echo "[4/5] Linking kernel..."
x86_64-linux-gnu-ld -m elf_i386 -T kernel/linker.ld \
    kernel_entry.o interrupts.o switch_task.o \
    idt.o pic.o pmm.o kmalloc.o libc.o syscalls_posix.o \
    keyboard.o thread.o timer.o task_scheduler.o event_channel.o vmm.o cow.o tss.o syscall_gate.o usermode.o ata.o vfs.o fat16.o elf_loader.o rtc.o pci.o memory_validator.o mouse.o bga.o ahci.o disk.o page_cache.o \
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
x86_64-linux-gnu-g++ -m32 -ffreestanding -fno-pie -fno-exceptions -fno-rtti -nostdlib -Iuser/libc/include -c user/libc/src/stdlib.cpp -o user_libc_stdlib.o
x86_64-linux-gnu-g++ -m32 -ffreestanding -fno-pie -fno-exceptions -fno-rtti -nostdlib -Iuser/libc/include -c user/libc/src/math.cpp -o user_libc_math.o
x86_64-linux-gnu-g++ -m32 -ffreestanding -fno-pie -fno-exceptions -fno-rtti -nostdlib -Iuser/libc/include -c user/libc/src/cxx.cpp -o user_libc_cxx.o
x86_64-linux-gnu-ar rcs user_libc.a user_libc_syscall.o user_libc_errno.o user_libc_string.o user_libc_malloc.o user_libc_stdio.o user_libc_stdlib.o user_libc_math.o user_libc_cxx.o

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

x86_64-linux-gnu-g++ -m32 -ffreestanding -fno-pie -fno-exceptions -fno-rtti -nostdlib -nostdinc -Iuser/libc/include -c user/stdlib_test.cpp -o user_stdlib_test.o
x86_64-linux-gnu-ld -m elf_i386 -T user/user.ld user_crt0.o user_stdlib_test.o user_libc.a -o STDLIBT.ELF
mcopy -i data.img STDLIBT.ELF ::/STDLIBT.ELF

x86_64-linux-gnu-g++ -m32 -ffreestanding -fno-pie -fno-exceptions -fno-rtti -nostdlib -nostdinc -fno-builtin -Iuser/libc/include -c user/mathtest.cpp -o user_mathtest.o
x86_64-linux-gnu-ld -m elf_i386 -T user/user.ld user_crt0.o user_mathtest.o user_libc.a -o MATHTEST.ELF
mcopy -i data.img MATHTEST.ELF ::/MATHTEST.ELF

x86_64-linux-gnu-g++ -m32 -ffreestanding -fno-pie -fno-exceptions -fno-rtti -nostdlib -nostdinc -Iuser/libc/include -c user/filetest.cpp -o user_filetest.o
x86_64-linux-gnu-ld -m elf_i386 -T user/user.ld user_crt0.o user_filetest.o user_libc.a -o FILETST.ELF
mcopy -i data.img FILETST.ELF ::/FILETST.ELF

x86_64-linux-gnu-g++ -m32 -ffreestanding -fno-pie -fno-exceptions -fno-rtti -nostdlib -nostdinc -Iuser/libc/include -c user/forktest.cpp -o user_forktest.o
x86_64-linux-gnu-ld -m elf_i386 -T user/user.ld user_crt0.o user_forktest.o user_libc.a -o FORKTST.ELF
mcopy -i data.img FORKTST.ELF ::/FORKTST.ELF

x86_64-linux-gnu-g++ -m32 -ffreestanding -fno-pie -fno-exceptions -fno-rtti -nostdlib -nostdinc -Iuser/libc/include -c user/fileio.cpp -o user_fileio.o
x86_64-linux-gnu-ld -m elf_i386 -T user/user.ld user_crt0.o user_fileio.o user_libc.a -o FILEIO.ELF
mcopy -i data.img FILEIO.ELF ::/FILEIO.ELF

x86_64-linux-gnu-g++ -m32 -ffreestanding -fno-pie -fno-exceptions -fno-rtti -nostdlib -Iuser -c user/anim.cpp -o user_anim.o
x86_64-linux-gnu-ld -m elf_i386 -T user/user.ld user_crt0.o user_anim.o -o ANIM.ELF
mcopy -i data.img ANIM.ELF ::/ANIM.ELF

x86_64-linux-gnu-g++ -m32 -ffreestanding -fno-pie -fno-exceptions -fno-rtti -nostdlib -nostdinc -Iuser -Iuser/libc/include -c user/ps2_driver.cpp -o user_ps2_driver.o
x86_64-linux-gnu-g++ -m32 -ffreestanding -fno-pie -fno-exceptions -fno-rtti -nostdlib -nostdinc -Iuser -Iuser/libc/include -c user/ps2_test.cpp -o user_ps2_test.o
x86_64-linux-gnu-ld -m elf_i386 -T user/user.ld user_crt0.o user_ps2_test.o user_ps2_driver.o user_libc.a -o PS2TEST.ELF
mcopy -i data.img PS2TEST.ELF ::/PS2TEST.ELF

# === Dynamic Linker (ld.so) ===
nasm -f elf32 user/ldso/ldso_entry.asm -o ldso_entry.o
x86_64-linux-gnu-g++ -m32 -fPIC -ffreestanding -fno-exceptions -fno-rtti -nostdlib -c user/ldso/ldso.cpp -o ldso_main.o
x86_64-linux-gnu-ld -m elf_i386 -pie -e _start --no-dynamic-linker -T user/ldso/ldso.ld ldso_entry.o ldso_main.o -o LD.SO
mcopy -i data.img LD.SO ::/LD.SO

# === Test Shared Library (libtest.so) ===
x86_64-linux-gnu-g++ -m32 -fPIC -ffreestanding -fno-exceptions -fno-rtti -nostdlib -c user/libtest/libtest.cpp -o libtest.o
x86_64-linux-gnu-ld -m elf_i386 -shared -T user/libtest/libtest.ld libtest.o -o LIBTEST.SO
mcopy -i data.img LIBTEST.SO ::/LIBTEST.SO

# === Dynamic Test App ===
x86_64-linux-gnu-g++ -m32 -ffreestanding -fno-pie -fno-exceptions -fno-rtti -nostdlib -Iuser -c user/dyntest.cpp -o user_dyntest.o
x86_64-linux-gnu-ld -m elf_i386 -T user/user.ld --dynamic-linker=/LD.SO user_crt0.o user_dyntest.o LIBTEST.SO -o DYNTEST.ELF
mcopy -i data.img DYNTEST.ELF ::/DYNTEST.ELF

echo "=== Building ISO Image ==="
mkdir -p iso_root
cp disk.img iso_root/
cp data.img iso_root/

if command -v mkisofs &> /dev/null; then
    mkisofs -R -J -b disk.img -c boot.catalog -o vlsmc.iso iso_root/
elif command -v genisoimage &> /dev/null; then
    genisoimage -R -J -b disk.img -c boot.catalog -o vlsmc.iso iso_root/
else
    echo "Warning: mkisofs/genisoimage not found. Falling back to build_iso.py"
    python3 build_iso.py disk.img vlsmc.iso
fi

rm -rf iso_root

echo ""
echo "DONE! To run:"
echo "qemu-system-i386 -fda disk.img -hda data.img -boot a"
