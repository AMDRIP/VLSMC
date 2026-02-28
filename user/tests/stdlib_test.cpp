#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

int main() {
    printf("=== STDLIB.H TEST ===\n");

    const char* num_str = "  -12345";
    int parsed = atoi(num_str);
    printf("atoi(\"%s\") = %d\n", num_str, parsed);

    char buf[32];
    itoa(parsed, buf, 10);
    printf("itoa(%d, 10) = %s\n", parsed, buf);

    itoa(255, buf, 16);
    printf("itoa(255, 16) = %s\n", buf);
    
    itoa(255, buf, 2);
    printf("itoa(255, 2) = %s\n", buf);

    int a = -42;
    printf("abs(%d) = %d\n", a, abs(a));

    div_t d = div(100, 3);
    printf("div(100, 3) = quot: %d, rem: %d\n", d.quot, d.rem);

    printf("Random numbers: ");
    srand(42);
    for (int i = 0; i < 5; i++) {
        printf("%d ", rand() % 100);
    }
    printf("\n");

    void* ptr = malloc(64);
    if (ptr) {
        printf("malloc() via stdlib/malloc.h successful: %p\n", ptr);
        free(ptr);
    }

    printf("Calling exit(0)...\n");
    exit(0);

    printf("This should not be printed.\n");
    return 1;
}
