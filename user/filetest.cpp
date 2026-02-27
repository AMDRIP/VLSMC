#include <stdio.h>
#include <string.h>

int main() {
    printf("=== FILE I/O TEST ===\n");

    FILE* fw = fopen("HELLO.TXT", "w");
    if (fw) {
        const char* msg = "Hello from VLSMC file I/O!";
        size_t written = fwrite(msg, 1, strlen(msg), fw);
        printf("Written %d bytes to HELLO.TXT\n", (int)written);
        fclose(fw);
    } else {
        printf("[FAIL] Could not open HELLO.TXT for writing\n");
    }

    FILE* fr = fopen("HELLO.TXT", "r");
    if (fr) {
        char buf[128];
        memset(buf, 0, sizeof(buf));
        size_t rd = fread(buf, 1, sizeof(buf) - 1, fr);
        printf("Read %d bytes: \"%s\"\n", (int)rd, buf);
        fclose(fr);
    } else {
        printf("[FAIL] Could not open HELLO.TXT for reading\n");
    }

    printf("=== FILE I/O TEST COMPLETE ===\n");
    return 0;
}
