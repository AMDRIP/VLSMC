#!/bin/bash
qemu-system-i386 \
  -drive file=disk.img,format=raw,if=floppy \
  -drive file=data.img,format=raw,if=ide \
  -boot a -nographic -no-reboot \
  -netdev user,id=net0 -device e1000,netdev=net0 \
  -d int -D qemu.log > qemu_stdout.log 2>&1 &
QEMU_PID=$!
sleep 2
echo " anim "
