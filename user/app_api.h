#pragma once

#include "syscalls.h"
#include <malloc.h>

#define FMODE_READ  1
#define FMODE_WRITE 2

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

    static int open(const char* path, int mode) {
        return sys_fopen(path, mode);
    }

    static int read(int fd, void* buf, uint32_t size) {
        return sys_fread(fd, buf, size);
    }

    static int write(int fd, const void* buf, uint32_t size) {
        return sys_fwrite(fd, buf, size);
    }

    static int close(int fd) {
        return sys_fclose(fd);
    }

    static uint32_t file_size(int fd) {
        return sys_fsize(fd);
    }

    static int fork() {
        return sys_fork();
    }

    static int exec(const char* path, char* const* argv = nullptr, char* const* envp = nullptr) {
        return sys_exec(path, argv, envp);
    }

    static int wait(int* status = nullptr) {
        return sys_wait(status);
    }

    static uint8_t inb(uint16_t port) {
        return sys_inb(port);
    }

    static void outb(uint16_t port, uint8_t val) {
        sys_outb(port, val);
    }

    static uint16_t inw(uint16_t port) {
        return sys_inw(port);
    }

    static void outw(uint16_t port, uint16_t val) {
        sys_outw(port, val);
    }

    static void wait_irq(uint8_t irq) {
        sys_wait_irq(irq);
    }

    static void* map_mmio(uint32_t virt_addr, uint32_t phys_addr, uint32_t size_pages) {
        return sys_map_mmio(virt_addr, phys_addr, size_pages);
    }

    static int grant_mmio(int tid, uint32_t phys_start, uint32_t phys_end) {
        return sys_grant_mmio(tid, phys_start, phys_end);
    }

    static int set_driver(int tid) {
        return sys_set_driver(tid);
    }

    static int msg_send(int target_tid, const void* buffer, uint32_t size) {
        return sys_send_msg(target_tid, buffer, size);
    }

    static int msg_recv(int* sender_tid_out, void* buffer, uint32_t max_size) {
        return sys_recv_msg(sender_tid_out, buffer, max_size);
    }

    static int find_thread(const char* name) {
        return sys_find_thread(name);
    }

    static bool read_sector(uint32_t lba, void* buffer) {
        return sys_read_sector(lba, buffer);
    }

    static void* sbrk(int increment) {
        return sys_sbrk(increment);
    }

    static void* malloc(size_t size) {
        return ::malloc(size);
    }

    static void free(void* ptr) {
        ::free(ptr);
    }
};

} // namespace vlsmc
