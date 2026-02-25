#pragma once

#include "syscalls.h"

struct DriverContext {
    void* private_data;
    size_t private_data_size;
    uint32_t device_id;
};

class DriverAPI {
public:
    virtual ~DriverAPI() = default;

    virtual int init(DriverContext* ctx) = 0;
    virtual int read(DriverContext* ctx, void* buffer, size_t size) = 0;
    virtual int write(DriverContext* ctx, const void* buffer, size_t size) = 0;
    virtual int ioctl(DriverContext* ctx, uint32_t cmd, void* arg) = 0;
    virtual void stop(DriverContext* ctx) = 0;

protected:
    uint8_t sys_inb(uint16_t port) {
        uint32_t ret;
        asm volatile("int $0x80" : "=a"(ret) : "a"(17), "b"(port));
        return (uint8_t)ret;
    }
    
    void sys_outb(uint16_t port, uint8_t data) {
        uint32_t dummy;
        asm volatile("int $0x80" : "=a"(dummy) : "a"(18), "b"(port), "c"(data));
    }
    
    uint16_t sys_inw(uint16_t port) {
        uint32_t ret;
        asm volatile("int $0x80" : "=a"(ret) : "a"(19), "b"(port));
        return (uint16_t)ret;
    }
    
    void sys_outw(uint16_t port, uint16_t data) {
        uint32_t dummy;
        asm volatile("int $0x80" : "=a"(dummy) : "a"(20), "b"(port), "c"(data));
    }
    
    void* sys_map_mmio(uint32_t virt_addr, uint32_t phys_addr, uint32_t flags) {
        uint32_t ret;
        asm volatile("int $0x80" : "=a"(ret) : "a"(21), "b"(virt_addr), "c"(phys_addr), "d"(flags));
        return (void*)ret;
    }
    
    void sys_wait_irq(uint8_t irq_num) {
        uint32_t dummy;
        asm volatile("int $0x80" : "=a"(dummy) : "a"(22), "b"(irq_num));
    }
};
