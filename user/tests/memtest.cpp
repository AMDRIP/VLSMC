#include <string.h>
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
    print("=== MEMORY TESTING ===\n");

    char buf1[32];
    char buf2[32];

    print("1. MEMSET TEST...\n");
    memset(buf1, 0xAA, 32);
    for (int i = 0; i < 32; i++) {
        check((unsigned char)buf1[i] == 0xAA, "MEMSET FAILED TO SET VALUE");
    }
    memset(buf1, 0x00, 32);
    check(buf1[0] == 0 && buf1[31] == 0, "MEMSET FAILED TO CLEAR BUFFER");
    print("   [SUCCESS]\n");

    print("2. MEMCPY TEST...\n");
    for (int i = 0; i < 32; i++) buf1[i] = i;
    memset(buf2, 0, 32);
    memcpy(buf2, buf1, 32);
    for (int i = 0; i < 32; i++) {
        check(buf2[i] == i, "MEMCPY COPIED INCORRECTLY");
    }
    print("   [SUCCESS]\n");

    print("3. MEMCMP TEST...\n");
    check(memcmp(buf1, buf2, 32) == 0, "MEMCMP FAILED TO COMPARE IDENTICAL BUFFERS");
    buf2[15] = 99;
    check(memcmp(buf1, buf2, 32) != 0, "MEMCMP FAILED TO FIND DIFFERENCE IN MIDDLE");
    buf2[15] = buf1[15];
    buf2[31] = 99;
    check(memcmp(buf1, buf2, 32) != 0, "MEMCMP FAILED TO FIND DIFFERENCE AT END");
    print("   [SUCCESS]\n");

    print("4. MEMMOVE TEST (NON-OVERLAPPING)...\n");
    char move_buf[64];
    for (int i = 0; i < 32; i++) move_buf[i] = i;
    for (int i = 32; i < 64; i++) move_buf[i] = 0;
    memmove(move_buf + 32, move_buf, 32);
    for (int i = 0; i < 32; i++) {
        check(move_buf[i + 32] == i, "MEMMOVE NORMAL COPY ERROR");
    }
    print("   [SUCCESS]\n");

    print("5. MEMMOVE TEST (FORWARD - OVERLAPPING)...\n");
    for (int i = 0; i < 64; i++) move_buf[i] = i;
    memmove(move_buf + 10, move_buf, 32);
    for (int i = 0; i < 32; i++) {
        check(move_buf[i + 10] == i, "MEMMOVE FORWARD OVERLAP ERROR DEST>SRC");
    }
    print("   [SUCCESS]\n");

    print("6. MEMMOVE TEST (BACKWARD - OVERLAPPING)...\n");
    for (int i = 0; i < 64; i++) move_buf[i] = i;
    memmove(move_buf, move_buf + 10, 32);
    for (int i = 0; i < 32; i++) {
        check(move_buf[i] == i + 10, "MEMMOVE BACKWARD OVERLAP ERROR DEST<SRC");
    }
    print("   [SUCCESS]\n");

    print("=== ALL TESTS PASSED ===\n");
    return 0;
}
