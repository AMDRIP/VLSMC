#pragma once

#include <stdint.h>

namespace re36 {

#define USER_CODE_VADDR  0x40048000
#define USER_STACK_VADDR 0xBFFFF000
#define USER_STACK_SIZE  4096

void enter_usermode();

} // namespace re36
