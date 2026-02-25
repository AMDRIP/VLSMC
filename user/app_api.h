#pragma once

#include "syscalls.h"

namespace vlsmc {

class App {
public:
    static void exit(int code = 0) {
        (void)code;
        sys_exit();
    }

    static void print(const char* str) {
        uint32_t len = 0;
        while (str[len]) len++;
        sys_print(str, len);
    }

    static char getchar() {
        return sys_getchar();
    }

    static void sleep(uint32_t ms) {
        sys_sleep(ms);
    }

    static void yield() {
        sys_yield();
    }

    static uint32_t get_pid() {
        return sys_getpid();
    }

    static uint32_t get_time() {
        return sys_time();
    }
};

} // namespace vlsmc
