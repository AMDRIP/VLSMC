#pragma once

#include "driver_api.h"

// PS/2 Device Types
#define PS2_DEVICE_KEYBOARD 0x01
#define PS2_DEVICE_MOUSE    0x02

class PS2Driver : public DriverAPI {
public:
    PS2Driver();
    virtual ~PS2Driver() = default;

    int init(DriverContext* ctx) override;
    int read(DriverContext* ctx, void* buffer, size_t size) override;
    int write(DriverContext* ctx, const void* buffer, size_t size) override;
    int ioctl(DriverContext* ctx, uint32_t cmd, void* arg) override;
    void stop(DriverContext* ctx) override;

private:
    uint8_t read_data_port();
    void write_data_port(uint8_t data);
    void write_command_port(uint8_t cmd);
    uint8_t read_status_port();
    
    void wait_read();
    void wait_write();
    
    // Commands
    static const uint16_t BASE_PORT_DATA = 0x60;
    static const uint16_t BASE_PORT_CMD = 0x64;
    
    bool is_mouse_;
};
