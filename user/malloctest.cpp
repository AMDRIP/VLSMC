#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void print_result(const char* test_name, int success) {
    if (success) {
        printf("[OK] %s\n", test_name);
    } else {
        printf("[FAIL] %s\n", test_name);
    }
}

int main() {
    printf("=== MALLOC V2 (FAST BINS + COALESCING) TEST ===\n");

    void* small1 = malloc(8);
    void* small2 = malloc(16);
    free(small1);
    void* small3 = malloc(8);
    print_result("Fast Bin Reuse", small1 == small3);
    free(small2);
    free(small3);

    void* a = malloc(200);
    void* b = malloc(300);
    void* c = malloc(200);

    memset(a, 'A', 200);
    memset(b, 'B', 300);
    memset(c, 'C', 200);

    free(a);
    free(c);

    free(b);

    void* giant = malloc(700);
    print_result("O(1) Boundary Tag Coalescing", giant == a);

    free(giant);

    void* huge1 = malloc(8192);
    void* huge2 = malloc(8192);
    free(huge1);
    free(huge2);

    void* coalesce_huge = malloc(16300);
    print_result("Contiguous sbrk() Coalescing", coalesce_huge == huge1);
    free(coalesce_huge);

    printf("=== ALL TESTS FINISHED ===\n");
    return 0;
}
