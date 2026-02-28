#pragma once

#include "syscalls.h"

struct DriverContext {
    void* private_data;
    size_t private_data_size;
    uint32_t device_id;
    uint8_t irq;
    uint32_t mmio_base;
    uint32_t mmio_size;
};

class DriverAPI {
public:
    virtual ~DriverAPI() = default;

    virtual int init(DriverContext* ctx) = 0;
    virtual int read(DriverContext* ctx, void* buffer, size_t size) = 0;
    virtual int write(DriverContext* ctx, const void* buffer, size_t size) = 0;
    virtual int ioctl(DriverContext* ctx, uint32_t cmd, void* arg) = 0;
    virtual void stop(DriverContext* ctx) = 0;
};
