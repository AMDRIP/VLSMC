#include "libc/include/string.h"
#include <sys/syscall.h>

void print(const char* str) {
    int len = 0;
    while (str[len]) len++;
    syscall(SYS_PRINT, (long)str, (long)len);
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
    print("=== ТЕСТИРОВАНИЕ СТРОК (ГРАНИЧНЫЕ СЛУЧАИ) ===\n");

    print("1. STRLEN / STRNLEN...\n");
    check(strlen(nullptr) == 0, "STRLEN НЕВЕРНО ОБРАБОТАЛ NULL");
    check(strnlen(nullptr, 10) == 0, "STRNLEN НЕВЕРНО ОБРАБОТАЛ NULL");
    check(strlen("") == 0, "STRLEN НЕВЕРНО ОБРАБОТАЛ ПУСТУЮ СТРОКУ");
    check(strnlen("test", 2) == 2, "STRNLEN НЕВЕРНО ОГРАНИЧИЛ ДЛИНУ (МЕНЬШЕ ИСХОДНОЙ)");
    check(strnlen("test", 10) == 4, "STRNLEN НЕВЕРНО ОГРАНИЧИЛ ДЛИНУ (БОЛЬШЕ ИСХОДНОЙ)");
    print("   [УСПЕХ]\n");

    print("2. STRCPY / STRLCPY...\n");
    char dest[10];
    check(strcpy(nullptr, "test") == nullptr, "STRCPY НЕВЕРНО ОБРАБОТАЛ DEST NULL");
    check(strcpy(dest, nullptr) == dest, "STRCPY НЕВЕРНО ОБРАБОТАЛ SRC NULL");
    
    memset(dest, 'X', 10);
    check(strlcpy(dest, "1234567890", 5) == 10, "STRLCPY ВЕРНУЛ НЕВЕРНУЮ ДЛИНУ SRC");
    check(dest[0] == '1' && dest[3] == '4', "STRLCPY НЕ СКОПИРОВАЛ ДАННЫЕ");
    check(dest[4] == '\0', "STRLCPY НЕ УСТАНОВИЛ НУЛЬ-ТЕРМИНАТОР ПОСЛЕ ОБРЕЗКИ");
    check(dest[5] == 'X', "STRLCPY ВЫШЕЛ ЗА УКАЗАННЫЕ ГРАНИЦЫ");

    check(strlcpy(dest, "12", 5) == 2, "STRLCPY ВЕРНУЛ НЕВЕРНУЮ ДЛИНУ SRC (КОРОТКАЯ СТРОКА)");
    check(dest[2] == '\0', "STRLCPY НЕ УСТАНОВИЛ НУЛЬ-ТЕРМИНАТОР (КОРОТКАЯ СТРОКА)");
    print("   [УСПЕХ]\n");

    print("3. STRCMP / STRNCMP / STRNCMP_S...\n");
    check(strcmp(nullptr, "test") == 0, "STRCMP НЕВЕРНО ОБРАБОТАЛ S1 NULL");
    check(strcmp("test", nullptr) == 0, "STRCMP НЕВЕРНО ОБРАБОТАЛ S2 NULL");
    check(strcmp("abc", "abc") == 0, "STRCMP НЕВЕРНО СРАВНИЛ ОДИНАКОВЫЕ СТРОКИ");
    check(strcmp("abc", "abd") < 0, "STRCMP НЕВЕРНО СРАВНИЛ (МЕНЬШЕ)");
    check(strcmp("abd", "abc") > 0, "STRCMP НЕВЕРНО СРАВНИЛ (БОЛЬШЕ)");
    check(strcmp("abc", "ab") > 0, "STRCMP НЕВЕРНО СРАВНИЛ РАЗЛИЧНЫЕ ДЛИНЫ (1)");
    check(strcmp("ab", "abc") < 0, "STRCMP НЕВЕРНО СРАВНИЛ РАЗЛИЧНЫЕ ДЛИНЫ (2)");
    
    check(strncmp("abcd", "abce", 3) == 0, "STRNCMP НЕВЕРНО ОГРАНИЧИЛ СРАВНЕНИЕ");
    check(strncmp("abcd", "abce", 4) < 0, "STRNCMP НЕВЕРНО СРАВНИЛ В ПРЕДЕЛАХ N");
    check(strncmp("ab", "abc", 5) < 0, "STRNCMP НЕВЕРНО СРАВНИЛ КОРОТКИЕ СТРОКИ ПРИ БОЛЬШОМ N");
    
    check(strncmp_s("abcd", 4, "abce", 3) == 0, "STRNCMP_S НЕВЕРНО ВЫБРАЛ МИНИМАЛЬНЫЙ ЛИМИТ");
    check(strncmp_s("abcd", 4, "abce", 4) < 0, "STRNCMP_S НЕВЕРНО СРАВНИЛ С ОДИНАКОВЫМИ ЛИМИТАМИ");
    print("   [УСПЕХ]\n");

    print("4. STRCHR / STRSTR...\n");
    const char* target = "hello world";
    check(strchr(nullptr, 'w') == nullptr, "STRCHR НЕВЕРНО ОБРАБОТАЛ СТРОКУ NULL");
    check(strchr(target, 'x') == nullptr, "STRCHR НАШЕЛ ОТСУТСТВУЮЩИЙ СИМВОЛ");
    check(strchr(target, 'w') == target + 6, "STRCHR НЕ НАШЕЛ СИМВОЛ В СЕРЕДИНЕ");
    check(strchr(target, '\0') == target + 11, "STRCHR НЕ НАШЕЛ НУЛЬ-ТЕРМИНАТОР");

    check(strstr(nullptr, "world") == nullptr, "STRSTR НЕВЕРНО ОБРАБОТАЛ HAYSTACK NULL");
    check(strstr(target, nullptr) == nullptr, "STRSTR НЕВЕРНО ОБРАБОТАЛ NEEDLE NULL");
    check(strstr(target, "") == target, "STRSTR НЕВЕРНО НАШЕЛ ПУСТУЮ СТРОКУ");
    check(strstr(target, "world") == target + 6, "STRSTR НЕ НАШЕЛ ПОДСТРОКУ В СЕРЕДИНЕ");
    check(strstr(target, "universe") == nullptr, "STRSTR НАШЕЛ ОТСУТСТВУЮЩУЮ ПОДСТРОКУ");
    check(strstr(target, "hello world!") == nullptr, "STRSTR НАШЕЛ БОЛЕЕ ДЛИННУЮ ПОДСТРОКУ");
    print("   [УСПЕХ]\n");

    print("=== ВСЕ ТЕСТЫ СТРОК ПРОЙДЕНЫ ===\n");
    return 0;
}
