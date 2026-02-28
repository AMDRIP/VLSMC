import re
import sys

# Using Linux paths since we run via wsl python3
path = "/mnt/y/DEV/VLSMC/build.sh"
with open(path, "r") as f:
    text = f.read()

# Add test folder creation if absent
if "mmd -i data.img ::/tests" not in text:
    text = text.replace('nasm -f elf32 user/crt0.asm -o user_crt0.o', 'nasm -f elf32 user/crt0.asm -o user_crt0.o\nmmd -i data.img ::/tests')

# Replace mcopy commands
def repl(m):
    filename = m.group(1)
    if filename in ["HELLO.ELF", "LD.SO", "LIBTEST.SO", "LIBC.SO"]:
        return m.group(0)
    return f"mcopy -i data.img {filename} ::/tests/{filename}"

text = re.sub(r'mcopy \-i data\.img ([a-zA-Z0-9_]+\.(?:ELF|SO)) ::/\1', repl, text)

with open(path, "w") as f:
    f.write(text)
print("Finished patching build.sh")
