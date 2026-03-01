#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <app_api.h>

void test_sprintf() {
    char buf[128];
    printf("Testing sprintf...\n");
    
    int count = sprintf(buf, "Num: %d, Str: %s, Hex: 0x%X", -42, "hello", 255);
    printf("  sprintf returned: %d\n", count);
    printf("  sprintf result: '%s'\n", buf);
    assert(strcmp(buf, "Num: -42, Str: hello, Hex: 0xFF") == 0);
    
    count = snprintf(buf, 10, "1234567890123");
    printf("  snprintf returned: %d (expected 13)\n", count);
    printf("  snprintf result (size=10): '%s'\n", buf);
    assert(strcmp(buf, "123456789") == 0); // 9 chars + null terminator
}

void test_sscanf() {
    printf("Testing sscanf...\n");
    int i;
    unsigned int x;
    char str[32];
    
    int parsed = sscanf("  -123 0xABC test_string  ", "%d %x %s", &i, &x, str);
    printf("  parsed %d items: i=%d, x=0x%X, str='%s'\n", parsed, i, x, str);
    
    assert(parsed == 3);
    assert(i == -123);
    assert(x == 0xABC);
    assert(strcmp(str, "test_string") == 0);
}

void test_file_io() {
    printf("Testing fprintf/fscanf and fseek...\n");
    
    FILE* f = fopen("/test_io.txt", "w");
    assert(f != nullptr);
    
    fprintf(f, "Line 1: %d\n", 100);
    fprintf(f, "Line 2: %X\n", 255);
    fprintf(f, "Word1 Word2 12345\n");
    fclose(f);
    
    f = fopen("/test_io.txt", "r");
    assert(f != nullptr);
    
    char word[32];
    int val;
    
    fscanf(f, "%s %s %d", word, word, &val); // Line 1: 100
    assert(val == 100);
    
    // Seek back to start
    fseek(f, 0, SEEK_SET);
    assert(ftell(f) == 0);
    
    char line[64];
    fscanf(f, "%s %s %d", word, line, &val); // Reread
    assert(val == 100);
    
    // Test seek CUR
    fseek(f, 8, SEEK_CUR); // skip "Line 2: "
    fscanf(f, "%x", &val);
    assert(val == 0xFF);
    
    fclose(f);
    printf("  File IO tests passed!\n");
}

int main(int argc, char** argv) {
    printf("\n=== RUNNING Advanced STDIO Tests ===\n");
    test_sprintf();
    test_sscanf();
    test_file_io();
    printf("=== All STDIO tests passed! ===\n\n");
    return 0;
}
