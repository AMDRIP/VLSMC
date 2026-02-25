#pragma once

#include <stdint.h>

namespace re36 {

class ShellAutocomplete {
public:
    static const char* complete(const char* partial);
};

} // namespace re36
