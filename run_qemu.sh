#!/bin/bash
qemu-system-i386 -fda disk.img -hda data.img -boot a -nographic -no-reboot -d int -D qemu.log > qemu_stdout.log 2>&1 &
QEMU_PID=$!
sleep 2
echo " anim 
