#pragma once

#include <stdint.h>
#include <stddef.h>

namespace re36 {

class MemoryValidator {
public:
    // Запускает все тесты памяти.
    // Возвращает true, если тесты пройдены успешно, иначе false.
    static bool run_all_tests();
    static bool test_pmm();
    static bool test_vmm();

private:
    static bool test_heap();
};

} // namespace re36
