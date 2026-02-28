import re

path = "/mnt/y/DEV/VLSMC/build.sh"
with open(path, "r") as f:
    text = f.read()

test_files = [
    "anim.cpp", "cat_test.cpp", "dynhello.cpp", "dyntest.cpp", "fileio.cpp", 
    "filetest.cpp", "forktest.cpp", "heap_test.cpp", "malloctest.cpp", 
    "mathtest.cpp", "memtest.cpp", "ps2_test.cpp", "stack_bomb.cpp", 
    "stdio_test.cpp", "stdlib_test.cpp", "strtest.cpp", "syscall_test.cpp", 
    "vesa_test.cpp"
]

for filename in test_files:
    text = text.replace(f"-c user/{filename}", f"-c user/tests/{filename}")

with open(path, "w") as f:
    f.write(text)

print("Updated source file locations.")
