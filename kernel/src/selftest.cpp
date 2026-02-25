#include "kernel/selftest.h"
#include "kernel/kmalloc.h"
#include "kernel/string.h"
#include "kernel/vector.h"
#include "libc.h"

namespace re36 {

static void panic_test(const char* msg) {
    printf("PANIC: %s\n", msg);
    while (true) asm volatile("hlt");
}

static void test_kmalloc() {
    printf("[TEST] kmalloc... ");
    void* ptr1 = kmalloc(128);
    void* ptr2 = kmalloc(256);
    if (!ptr1 || !ptr2 || ptr1 == ptr2) {
        panic_test("kmalloc failed test 1");
    }
    kfree(ptr1);
    kfree(ptr2);
    printf("OK\n");
}

static void test_string() {
    printf("[TEST] string... ");
    const char* s1 = "hello";
    const char* s2 = "hello";
    const char* s3 = "world";
    
    if (strlen(s1) != 5) {
        panic_test("strlen failed");
    }
    if (strcmp(s1, s2) != 0) {
        panic_test("strcmp failed 1");
    }
    if (strcmp(s1, s3) == 0) {
        panic_test("strcmp failed 2");
    }
    printf("OK\n");
}

static void test_vector() {
    printf("[TEST] vector... ");
    vector<int> v;
    for (int i = 0; i < 100; i++) {
        v.push_back(i * 2);
    }
    if (v.size() != 100) {
        panic_test("vector size is wrong");
    }
    for (size_t i = 0; i < 100; i++) {
        if (v[i] != (int)(i * 2)) {
            panic_test("vector data is wrong");
        }
    }
    printf("OK\n");
}

void run_all_tests() {
    printf("--- Running Kernel Self-Tests ---\n");
    test_kmalloc();
    test_string();
    test_vector();
    printf("[OK] All internal self-tests passed.\n\n");
}

}
