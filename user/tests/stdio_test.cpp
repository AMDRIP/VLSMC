#include "stdio.h"

int main() {
    printf("=== STDIO TEST ===\n");
    printf("Testing string: %s\n", "Hello, World!");
    printf("Testing integer: %d, negative: %d\n", 42, -123);
    printf("Testing hex: 0x%x, 0x%X\n", 0xABCD1234, 0x5678EF);
    printf("Testing pointer: %p\n", (void*)0xDEADBEEF);
    printf("Testing char: %c\n", 'A');
    printf("Testing percent: %%\n");

    printf("\nType something: ");
    char buf[64];
    if (gets_s(buf, sizeof(buf))) {
        printf("You typed: %s\n", buf);
    } else {
        printf("gets_s failed\n");
    }

    return 0;
}
