import os
import re

test_dir = "/mnt/y/DEV/VLSMC/user/tests"

for filename in os.listdir(test_dir):
    if filename.endswith(".cpp"):
        filepath = os.path.join(test_dir, filename)
        with open(filepath, "r", encoding="utf-8") as f:
            content = f.read()
            
        # Replace #include "libc/include/foo.h" or #include "../libc/include/foo.h" with #include <foo.h>
        original_content = content
        content = re.sub(r'#include\s+["<](?:\.\./)?(?:user/)?libc/include/([^">]+)[">]', r'#include <\1>', content)
        
        # Also fix "#include "sys/syscall.h"" -> "#include <sys/syscall.h>" if there are any that are failing
        content = re.sub(r'#include\s+"(sys/[^"]+)"', r'#include <\1>', content)
        
        if content != original_content:
            with open(filepath, "w", encoding="utf-8") as f:
                f.write(content)
            print(f"Fixed includes in {filename}")

print("Include fix completed.")
