#include "app_api.h"
#include "syscalls.h"

#define COM1_PORT 0x3F8

extern "C" void __cxa_pure_virtual() {
    vlsmc::App::print("Pure virtual function call!\n");
    vlsmc::App::exit(1);
}

void outb(uint16_t port, uint8_t val) {
    sys_outb(port, val);
}

uint8_t inb(uint16_t port) {
    return sys_inb(port);
}

void init_serial() {
    outb(COM1_PORT + 1, 0x00);    // Disable all interrupts
    outb(COM1_PORT + 3, 0x80);    // Enable DLAB (set baud rate divisor)
    outb(COM1_PORT + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
    outb(COM1_PORT + 1, 0x00);    //                  (hi byte)
    outb(COM1_PORT + 3, 0x03);    // 8 bits, no parity, one stop bit
    outb(COM1_PORT + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
    outb(COM1_PORT + 4, 0x0B);    // IRQs enabled, RTS/DSR set
}

int is_transmit_empty() {
    return inb(COM1_PORT + 5) & 0x20;
}

void write_serial(char a) {
    while (is_transmit_empty() == 0);
    outb(COM1_PORT, a);
}

void write_string(const char* str) {
    for (int i = 0; str[i] != '\0'; ++i) {
        write_serial(str[i]);
    }
}

int serial_received() {
    return inb(COM1_PORT + 5) & 1;
}

char read_serial() {
    while (serial_received() == 0) {
        // Wait for IRQ 4 to wake us up to save CPU cycles
        sys_wait_irq(4);
    }
    return inb(COM1_PORT);
}

int main() {
    vlsmc::App app;
    app.print("=== UART COM1 User-space Driver ===\n");
    
    // Explicit grants are obtained through shell execution mechanism
    sys_set_driver(sys_getpid());
    
    app.print("Initializing COM1 at 38400 baud...\n");
    init_serial();
    
    app.print("Writing test string to COM1...\n");
    write_string("Hello from VLSMC UART Driver!\r\n");
    
    app.print("Listening for incoming data on COM1 (press ESC to exit):\n");
    
    bool running = true;
    while (running) {
        char c = read_serial();
        
        char msg[2] = {c, '\0'};
        // Only print printable characters locally to avoid terminal corruption
        if (c >= 32 && c <= 126) {
            app.print(msg);
        } else if (c == '\r' || c == '\n') {
            app.print("\n");
        }
        
        write_serial(c); // Echo back to COM1
        
        if (c == 27) { // ESC key
            app.print("\nESC received. Exiting UART driver.\n");
            running = false;
        }
    }
    
    return 0;
}
