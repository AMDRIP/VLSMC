#include "syscalls.h"

extern "C" int test_add(int a, int b);
extern "C" int test_multiply(int a, int b);

static void dyn_print_num(int n) {
    char buf[12];
    int pos = 0;
    if (n == 0) { buf[0] = '0'; pos = 1; }
    else {
        if (n < 0) { buf[pos++] = '-'; n = -n; }
        char tmp[10]; int t = 0;
        while (n > 0) { tmp[t++] = '0' + (n % 10); n /= 10; }
        while (t > 0) buf[pos++] = tmp[--t];
    }
    buf[pos] = '\0';
    uint32_t len = 0;
    while (buf[len]) len++;
    syscall2(SYS_PRINT, (uint32_t)buf, len);
}

static void dyn_print(const char* s) {
    uint32_t len = 0;
    while (s[len]) len++;
    syscall2(SYS_PRINT, (uint32_t)s, len);
}

int main() {
    dyn_print("[DYNTEST] Dynamic linking test\n");

    int sum = test_add(3, 7);
    dyn_print("[DYNTEST] test_add(3, 7) = ");
    dyn_print_num(sum);
    dyn_print("\n");

    int prod = test_multiply(4, 5);
    dyn_print("[DYNTEST] test_multiply(4, 5) = ");
    dyn_print_num(prod);
    dyn_print("\n");

    if (sum == 10 && prod == 20) {
        dyn_print("[DYNTEST] SUCCESS: Dynamic linking works!\n");
    } else {
        dyn_print("[DYNTEST] FAIL: Wrong results\n");
    }

    return 0;
}
