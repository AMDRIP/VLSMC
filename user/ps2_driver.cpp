#include "ps2_driver.h"
#include "app_api.h"

PS2Driver::PS2Driver() : is_mouse_(false) {
}

void PS2Driver::wait_read() {
    int timeout = 100000;
    while (timeout--) {
        if ((sys_inb(BASE_PORT_CMD) & 1) != 0) {
            return;
        }
    }
}

void PS2Driver::wait_write() {
    int timeout = 100000;
    while (timeout--) {
        if ((sys_inb(BASE_PORT_CMD) & 2) == 0) {
            return;
        }
    }
}

uint8_t PS2Driver::read_status_port() {
    return sys_inb(BASE_PORT_CMD);
}

uint8_t PS2Driver::read_data_port() {
    wait_read();
    return sys_inb(BASE_PORT_DATA);
}

void PS2Driver::write_command_port(uint8_t cmd) {
    wait_write();
    sys_outb(BASE_PORT_CMD, cmd);
}

void PS2Driver::write_data_port(uint8_t data) {
    wait_write();
    sys_outb(BASE_PORT_DATA, data);
}

int PS2Driver::init(DriverContext* ctx) {
    if (!ctx) return -1;
    
    is_mouse_ = (ctx->device_id == PS2_DEVICE_MOUSE);
    
    // Disable devices
    write_command_port(0xAD);
    write_command_port(0xA7);

    // Flush output buffer
    sys_inb(BASE_PORT_DATA);

    // Read Controller Configuration Byte
    write_command_port(0x20);
    uint8_t config = read_data_port();

    // Enable interrupts and translation (for Keyboard)
    config |= 0x01; // First PS/2 port interrupt
    config |= 0x40; // First PS/2 port translation
    
    // Enable mouse IRQ if requested
    if (is_mouse_) {
        config |= 0x02; // Second PS/2 port interrupt
    }

    // Write back configuration
    write_command_port(0x60);
    write_data_port(config);

    // Enable devices
    write_command_port(0xAE);
    
    if (is_mouse_) {
        // Enable second PS/2 port forwarding
        write_command_port(0xA8); 
        
        // Tell mouse to use defaults and enable reporting
        write_command_port(0xD4);
        write_data_port(0xF6); // Set defaults
        read_data_port(); // ACK
        
        write_command_port(0xD4);
        write_data_port(0xF4); // Enable data reporting
        read_data_port(); // ACK
    }
    
    return 0;
}

int PS2Driver::read(DriverContext* ctx, void* buffer, size_t size) {
    if (!ctx || !buffer || size == 0) return -1;
    
    uint8_t* u8_buf = (uint8_t*)buffer;
    size_t bytes_read = 0;
    
    while (bytes_read < size) {
        // Wait for IRQ 1 (Keyboard) or IRQ 12 (Mouse)
        sys_wait_irq(is_mouse_ ? 12 : 1);
        
        // Verify there is data available in port 0x60
        if ((sys_inb(BASE_PORT_CMD) & 1) != 0) {
            u8_buf[bytes_read++] = sys_inb(BASE_PORT_DATA);
        }
    }
    
    return bytes_read;
}

int PS2Driver::write(DriverContext* ctx, const void* buffer, size_t size) {
    if (!ctx || !buffer || size == 0) return -1;

    const uint8_t* u8_buf = (const uint8_t*)buffer;
    for (size_t i = 0; i < size; i++) {
        if (is_mouse_) {
            write_command_port(0xD4);
        }
        write_data_port(u8_buf[i]);
    }

    return size;
}

int PS2Driver::ioctl(DriverContext* ctx, uint32_t cmd, void* arg) {
    return -1; // Not implemented yet
}

int PS2Driver::seek(DriverContext* ctx, uint32_t offset) {
    return -1; // Not seekable
}

void PS2Driver::stop(DriverContext* ctx) {
    write_command_port(0xAD);
    if (is_mouse_) {
        write_command_port(0xA7);
    }
}
