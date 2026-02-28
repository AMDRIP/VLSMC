static inline int syscall3(int syscall_num, int arg1, int arg2, int arg3) {
    int ret;
    asm volatile(
        "int $0x80"
        : "=a"(ret)
        : "a"(syscall_num), "b"(arg1), "c"(arg2), "d"(arg3)
        : "memory"
    );
    return ret;
}

void print_char(char c) {
    syscall3(4, 1, (int)&c, 1); // sys_write
}

void print_str(const char* str) {
    while (*str) {
        print_char(*str++);
    }
}

void print_num(int num) {
    if (num == 0) {
        print_char('0');
        return;
    }
    if (num < 0) {
        print_char('-');
        num = -num;
    }
    char buf[16];
    int pos = 0;
    while (num > 0) {
        buf[pos++] = '0' + (num % 10);
        num /= 10;
    }
    while (pos > 0) {
        print_char(buf[--pos]);
    }
}

void recursive_function(int depth) {
    if (depth % 100 == 0) {
        print_str("Recursion depth: ");
        print_num(depth);
        print_str("\n");
    }
    char big_array[1024]; // Ускоряет переполнение стека
    for (int i=0; i<1024; i++) big_array[i] = depth & 0xFF; // Access array to prevent optimization
    recursive_function(depth + 1);
}

int main() {
    print_str("Starting Stack Guard Test...\n");
    recursive_function(1);
    return 0;
}
