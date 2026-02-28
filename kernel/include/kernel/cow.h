#pragma once

#include <stdint.h>

namespace re36 {

uint32_t* cow_clone_directory();
bool cow_handle_fault(uint32_t fault_addr, uint32_t error_code);

} // namespace re36
