#pragma once

#include <stdint.h>

namespace re36 {

#define USER_ELF_MAX_SIZE  (32 * 1024)
#define USER_STACK_TOP     0xBFFF0000
#define USER_STACK_PAGES   4

int elf_exec(const char* filename, bool is_driver = false);

} // namespace re36
