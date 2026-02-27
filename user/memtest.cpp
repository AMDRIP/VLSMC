#include "libc/include/string.h"
#include <sys/syscall.h>

void print(const char* str) {
    int len = 0;
    while (str[len]) len++;
    syscall(SYS_PRINT, (long)str, (long)len);
}

void print_hex(unsigned int val) {
    char buf[11];
    buf[0] = '0';
    buf[1] = 'x';
    for (int i = 7; i >= 0; i--) {
        int nibble = (val >> (i * 4)) & 0xF;
        buf[9 - i] = nibble < 10 ? '0' + nibble : 'A' + (nibble - 10);
    }
    buf[10] = '\0';
    print(buf);
}

void check(bool condition, const char* msg) {
    if (!condition) {
        print("[СБОЙ] ");
        print(msg);
        print("\n");
        syscall(SYS_EXIT, 1, 0);
    }
}

int main() {
    print("=== ТЕСТИРОВАНИЕ ПАМЯТИ ===\n");

    char buf1[32];
    char buf2[32];

    print("1. TECT MEMSET...\n");
    memset(buf1, 0xAA, 32);
    for (int i = 0; i < 32; i++) {
        check((unsigned char)buf1[i] == 0xAA, "MEMSET НЕ УСТАНОВИЛ ЗНАЧЕНИЕ");
    }
    memset(buf1, 0x00, 32);
    check(buf1[0] == 0 && buf1[31] == 0, "MEMSET НЕ ОЧИСТИЛ БУФЕР");
    print("   [УСПЕХ]\n");

    print("2. TECT MEMCPY...\n");
    for (int i = 0; i < 32; i++) buf1[i] = i;
    memset(buf2, 0, 32);
    memcpy(buf2, buf1, 32);
    for (int i = 0; i < 32; i++) {
        check(buf2[i] == i, "MEMCPY СКОПИРОВАЛ НЕВЕРНО");
    }
    print("   [УСПЕХ]\n");

    print("3. TECT MEMCMP...\n");
    check(memcmp(buf1, buf2, 32) == 0, "MEMCMP НЕВЕРНО СРАВНИЛ ИДЕНТИЧНЫЕ БУФЕРЫ");
    buf2[15] = 99;
    check(memcmp(buf1, buf2, 32) != 0, "MEMCMP НЕ НАШЕЛ РАЗЛИЧИЕ В СЕРЕДИНЕ");
    buf2[15] = buf1[15];
    buf2[31] = 99;
    check(memcmp(buf1, buf2, 32) != 0, "MEMCMP НЕ НАШЕЛ РАЗЛИЧИЕ В КОНЦЕ");
    print("   [УСПЕХ]\n");

    print("4. TECT MEMMOVE (НЕПЕРЕСЕКАЮЩИЕСЯ)...\n");
    char move_buf[64];
    for (int i = 0; i < 32; i++) move_buf[i] = i;
    for (int i = 32; i < 64; i++) move_buf[i] = 0;
    memmove(move_buf + 32, move_buf, 32);
    for (int i = 0; i < 32; i++) {
        check(move_buf[i + 32] == i, "MEMMOVE ОШИБКА ОБЫЧНОГО КОПИРОВАНИЯ");
    }
    print("   [УСПЕХ]\n");

    print("5. TECT MEMMOVE (ВПЕРЕД - ПЕРЕСЕЧЕНИЕ)...\n");
    for (int i = 0; i < 64; i++) move_buf[i] = i;
    memmove(move_buf + 10, move_buf, 32);
    for (int i = 0; i < 32; i++) {
        check(move_buf[i + 10] == i, "MEMMOVE ОШИБКА ПЕРЕСЕЧЕНИЯ ВПЕРЕД DEST>SRC");
    }
    print("   [УСПЕХ]\n");

    print("6. TECT MEMMOVE (НАЗАД - ПЕРЕСЕЧЕНИЕ)...\n");
    for (int i = 0; i < 64; i++) move_buf[i] = i;
    memmove(move_buf, move_buf + 10, 32);
    for (int i = 0; i < 32; i++) {
        check(move_buf[i] == i + 10, "MEMMOVE ОШИБКА ПЕРЕСЕЧЕНИЯ НАЗАД DEST<SRC");
    }
    print("   [УСПЕХ]\n");

    print("=== ВСЕ ТЕСТЫ ПРОЙДЕНЫ ===\n");
    return 0;
}
