#include "libc/include/string.h"
#include "libc/include/malloc.h"
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
        print("[FAIL] ");
        print(msg);
        print("\n");
        syscall(SYS_EXIT, 1, 0);
    }
}

int main() {
    print("=== MALLOC TESTING ===\n");

    print("1. BASIC ALLOCATION...\n");
    void* ptr1 = malloc(32);
    check(ptr1 != nullptr, "MALLOC RETURNED NULL");
    memset(ptr1, 0xAA, 32);
    check(((unsigned char*)ptr1)[15] == 0xAA, "MALLOC MEMORY CORRUPTED");
    print("   [SUCCESS]\n");

    print("2. MULTIPLE ALLOCATIONS...\n");
    void* ptr2 = malloc(64);
    void* ptr3 = malloc(128);
    check(ptr2 != nullptr && ptr3 != nullptr, "MALLOC MULTIPLE FAIL");
    check(ptr1 != ptr2 && ptr2 != ptr3 && ptr1 != ptr3, "MALLOC RETURNED OVERLAPPING POINTERS");
    print("   [SUCCESS]\n");

    print("3. FREE AND REUSE (COALESCING LIST)...\n");
    free(ptr2);
    void* ptr4 = malloc(32);
    check(ptr4 != nullptr, "MALLOC REUSE FAIL");
    print("   [SUCCESS]\n");

    print("4. CALLOC TEST...\n");
    unsigned int* cptr = (unsigned int*)calloc(4, sizeof(unsigned int));
    check(cptr != nullptr, "CALLOC FAIL");
    check(cptr[0] == 0 && cptr[3] == 0, "CALLOC DID NOT ZERO MEMORY");
    print("   [SUCCESS]\n");

    print("5. REALLOC EDGE CASES...\n");
    void* r_null = realloc(nullptr, 16);
    check(r_null != nullptr, "REALLOC(NULL) DOES NOT WORK AS MALLOC");
    
    void* r_zero = realloc(r_null, 0);
    check(r_zero == nullptr, "REALLOC(PTR, 0) DOES NOT RETURN NULL");

    void* rptr = realloc(cptr, 128);
    check(rptr != nullptr, "REALLOC GROW FAIL");

    void* rptr_shrink = realloc(rptr, 32);
    check(rptr_shrink != nullptr, "REALLOC SHRINK FAIL");
    print("   [SUCCESS]\n");

    print("6. MALLOC(0) AND MASSIVE ALLOCATION...\n");
    void* zero_ptr = malloc(0);
    check(zero_ptr == nullptr, "MALLOC(0) MUST RETURN NULL");

    void* massive = malloc(1024 * 1024); // 1 MB
    check(massive != nullptr, "MASSIVE ALLOCATION FAILED (SYS_SBRK ISSUE)");
    ((char*)massive)[1024 * 1024 - 1] = 0xBB;
    check(((char*)massive)[1024 * 1024 - 1] == (char)0xBB, "MASSIVE MEMORY CORRUPTED");
    free(massive);
    print("   [SUCCESS]\n");

    print("7. COALESCING LEFT AND RIGHT...\n");
    void* a = malloc(64);
    void* b = malloc(64);
    void* c = malloc(64);
    
    free(a);
    free(c);
    free(b); // Should coalesce with a (left) and c (right)

    void* giant = malloc(180); // Should fit perfectly into the coalesced A+B+C block
    check(giant != nullptr, "COALESCING LEFT/RIGHT FAILED");
    check(giant == a, "COALESCING DID NOT REUSE THE EXACT BLOCK START");
    free(giant);
    print("   [SUCCESS]\n");

    free(ptr1);
    free(ptr3);
    free(ptr4);
    free(rptr_shrink);

    print("=== ALL TESTS PASSED ===\n");
    return 0;
}
