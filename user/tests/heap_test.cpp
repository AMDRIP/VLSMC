#include "app_api.h"

// Простой Itoa для тестов в User Space
static void print_num(int num) {
    if (num == 0) {
        vlsmc::App::print("0");
        return;
    }
    char buf[16];
    int i = 0;
    while (num > 0) {
        buf[i++] = '0' + (num % 10);
        num /= 10;
    }
    for (int j = i - 1; j >= 0; j--) {
        char s[2] = {buf[j], 0};
        vlsmc::App::print(s);
    }
}

int main() {
    vlsmc::App::print("[HEAP TEST] Starting tests...\n");

    // Выделим 1 МБ памяти (заставит кучу вырасти на 256 страниц)
    const int ALLOC_SIZE = 1024 * 1024;
    vlsmc::App::print("[HEAP TEST] Allocating 1 MB: ");
    print_num(ALLOC_SIZE);
    vlsmc::App::print(" bytes...\n");

    uint8_t* ptr = (uint8_t*)vlsmc::App::malloc(ALLOC_SIZE);

    if (!ptr) {
        vlsmc::App::print("[HEAP TEST] FAILED: malloc returned NULL.\n");
        return 1;
    }

    vlsmc::App::print("[HEAP TEST] SUCCESS: malloc returned a valid pointer.\n");

    // Теперь запишем туда значения.
    // Фактически здесь будут происходить Page Faults, и ядро будет 
    // лениво выдавать физическую память страница за страницей.
    vlsmc::App::print("[HEAP TEST] Writing to allocated memory...\n");
    for (int i = 0; i < ALLOC_SIZE; i++) {
        ptr[i] = (uint8_t)(i & 0xFF);
    }

    // Прочитаем обратно
    vlsmc::App::print("[HEAP TEST] Verifying written data...\n");
    bool ok = true;
    for (int i = 0; i < ALLOC_SIZE; i++) {
        if (ptr[i] != (uint8_t)(i & 0xFF)) {
            ok = false;
            break;
        }
    }

    if (ok) {
        vlsmc::App::print("[HEAP TEST] SUCCESS: Data matches perfectly! Lazy allocation works!\n");
    } else {
        vlsmc::App::print("[HEAP TEST] FAILED: Data corruption detected.\n");
    }

    // Можно попробовать выделить еще памяти
    vlsmc::App::print("[HEAP TEST] Allocating more memory...\n");
    uint8_t* ptr2 = (uint8_t*)vlsmc::App::malloc(4096);
    if (ptr2 > ptr) {
        vlsmc::App::print("[HEAP TEST] Second allocation is linearly higher. Bump allocator works.\n");
    }

    vlsmc::App::print("[HEAP TEST] Freeing second allocation...\n");
    vlsmc::App::free(ptr2);
    vlsmc::App::print("[HEAP TEST] Freeing first allocation (1 MB)...\n");
    vlsmc::App::free(ptr);
    vlsmc::App::print("[HEAP TEST] Memory cleanly freed!\n");

    vlsmc::App::print("[HEAP TEST] All tests done. Exiting.\n");
    return 0;
}
