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

    static int msg_send(int target_tid, const void* buffer, uint32_t size) {
        return sys_send_msg(target_tid, buffer, size);
    }

    static int msg_recv(int* sender_tid_out, void* buffer, uint32_t max_size) {
        return sys_recv_msg(sender_tid_out, buffer, max_size);
    }

    static bool read_sector(uint32_t lba, void* buffer) {
        return sys_read_sector(lba, buffer);
    }

    static void* map_mmio(uint32_t virt_addr, uint32_t phys_addr, uint32_t size_pages) {
        return sys_map_mmio(virt_addr, phys_addr, size_pages);
    }

    static int find_thread(const char* name) {
        return sys_find_thread(name);
    }

    static void* malloc(size_t size) {
        if (size == 0) return nullptr;
        // Выравнивание размера блока по 4 байтам
        size = (size + 3) & ~3;
        size_t total_size = size + sizeof(size_t);

        void* ptr = sys_sbrk(total_size);
        if ((uint32_t)ptr == (uint32_t)-1) return nullptr;

        size_t* meta = (size_t*)ptr;
        *meta = total_size;

        return (void*)(meta + 1);
    }

    static void free(void* ptr) {
        if (!ptr) return;
        
        size_t* meta = (size_t*)ptr - 1;
        size_t total_size = *meta;

        void* current_break = sys_sbrk(0);
        
        // Если освобождаем блок, который находится ровно в конце кучи, мы можем сузить кучу
        if ((uint8_t*)meta + total_size == (uint8_t*)current_break) {
            sys_sbrk(-((int)total_size));
        }
    }
};

} // namespace vlsmc
