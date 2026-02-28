#include "app_api.h"
#include "ps2_driver.h"
#include "syscalls.h"

// Scancode to ASCII mapping
static const char kbd_us_qwerty[128] = {
    0,  27, '1','2','3','4','5','6','7','8','9','0','-','=', '\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']', '\n',
    0, // Left Ctrl
    'a','s','d','f','g','h','j','k','l',';','\'','`',
    0, // Left Shift
    '\\','z','x','c','v','b','n','m',',','.','/',
    0, // Right Shift
    '*', 0, ' ', 0, // Alt, Space, CapsLock
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // F1-F10
    0, 0,
    '7','8','9','-','4','5','6','+','1','2','3','0','.'
};

int main() {
    vlsmc::App app;
    app.print("\n=== PS/2 User-Space Driver Test ===\n");
    app.print("Please note: The kernel's PS/2 driver is still active.\n");
    app.print("Key strokes will be captured by BOTH drivers.\n");
    app.print("Press 'ESC' (Scancode 0x01) to exit.\n\n");

    // Tell kernel we are a driver to get sys_inb / sys_outb permissions
    sys_set_driver(sys_getpid());

    PS2Driver ps2;
    DriverContext ctx;
    ctx.private_data = nullptr;
    ctx.private_data_size = 0;
    ctx.device_id = PS2_DEVICE_KEYBOARD; // We want to test keyboard

    app.print("Initializing PS2Driver...\n");
    if (ps2.init(&ctx) != 0) {
        app.print("PS2Driver initialization failed!\n");
        sys_exit();
    }
    app.print("PS2Driver initialized successfully!\n");

    bool running = true;
    while (running) {
        uint8_t scancode = 0;
        
        // Read 1 byte from PS/2 Keyboard
        int bytes_read = ps2.read(&ctx, &scancode, 1);
        
        if (bytes_read == 1) {
            bool is_release = (scancode & 0x80) != 0;
            uint8_t make_code = scancode & 0x7F;

            app.print("[UserPS2] Scancode: 0x");
            
            // Print Hex
            const char hex_chars[] = "0123456789ABCDEF";
            char hex[3];
            hex[0] = hex_chars[(scancode >> 4) & 0xF];
            hex[1] = hex_chars[scancode & 0xF];
            hex[2] = '\0';
            app.print(hex);
            
            if (is_release) {
                app.print(" (Released)");
            } else {
                app.print(" (Pressed)");
                if (make_code < 128) {
                    char ascii = kbd_us_qwerty[make_code];
                    if (ascii >= 32 && ascii <= 126) {
                        char msg[16] = " -> ' '";
                        msg[5] = ascii;
                        app.print(msg);
                    }
                }
            }
            app.print("\n");

            if (make_code == 0x01) { // ESC key
                app.print("ESC pressed. Exiting...\n");
                running = false;
            }
        }
    }

    ps2.stop(&ctx);
    app.print("PS2Driver stopped.\n");
    
    return 0;
}
