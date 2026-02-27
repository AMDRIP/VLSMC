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
    printf("=== STRING.H SWAR TEST SUITE ===\n");

    const char* str1 = "Hello";
    size_t len1 = strlen(str1);
    print_result("strlen (short)", len1 == 5);

    const char* str2 = "This is a much longer string exactly 44 chars";
    size_t len2 = strlen(str2);
    print_result("strlen (SWAR long)", len2 == 45);

    char buf[128];
    strcpy(buf, str2);
    print_result("strcpy (SWAR)", strcmp(buf, str2) == 0);

    strcat(buf, "!");
    print_result("strcat", strcmp(buf, "This is a much longer string exactly 44 chars!") == 0);

    char buf2[64];
    strlcpy(buf2, "Hello", sizeof(buf2));
    strlcat(buf2, " World", sizeof(buf2));
    print_result("strlcat", strcmp(buf2, "Hello World") == 0);

    char buf3[64] = "Hello World";
    char* found = strchr(buf3, 'W');
    print_result("strchr", found != NULL && found[0] == 'W');

    char* last_found = strrchr(buf3, 'o');
    print_result("strrchr", last_found != NULL && last_found == &buf3[7]);

    memset(buf, 'A', 60);
    buf[60] = '\0';
    print_result("memset (SWAR)", buf[0] == 'A' && buf[59] == 'A' && strlen(buf) == 60);

    memcpy(buf2, buf, 61);
    print_result("memcpy (SWAR aligned)", buf2[59] == 'A' && buf2[60] == '\0');

    memcpy(buf2 + 1, buf, 61);
    print_result("memcpy (SWAR unaligned dest)", buf2[60] == 'A' && buf2[61] == '\0');

    memmove(buf2 + 5, buf2 + 1, 10);
    print_result("memmove (overlapping)", buf2[5] == 'A' && buf2[14] == 'A');

    int cmp_res = memcmp("TestA", "TestB", 5);
    print_result("memcmp", cmp_res != 0);

    int cmp_s_res = memcmp_s("Test", 4, "Test", 4);
    print_result("memcmp_s", cmp_s_res == 0);

    printf("=== ALL TESTS FINISHED ===\n");
    return 0;
}
