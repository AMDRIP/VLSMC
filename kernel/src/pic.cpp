#include "kernel/pic.h"

#define PIC1_COMMAND 0x20
#define PIC1_DATA    0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA    0xA1

#define ICW1_ICW4       0x01    /* ICW4 (not) needed */
#define ICW1_INIT       0x10    /* Initialization - required! */
#define ICW4_8086       0x01    /* 8086/88 (MCS-80/85) mode */

namespace re36 {

void pic_send_eoi(uint8_t irq) {
    if (irq >= 8) {
        outb(PIC2_COMMAND, 0x20); // EOI для Slave PIC
    }
    outb(PIC1_COMMAND, 0x20); // EOI для Master PIC
}

void pic_remap(int offset1, int offset2) {
    uint8_t a1, a2;
    
    // Сохраняем текущие маски
    a1 = inb(PIC1_DATA);
    a2 = inb(PIC2_DATA);
    
    // Начинаем инициализацию (ICW1)
    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4); io_wait();
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4); io_wait();
    
    // ICW2: Устанавливаем оффсеты (смещения в IDT)
    outb(PIC1_DATA, offset1); io_wait(); // IRQ0..IRQ7 -> offset1 (обычно 0x20)
    outb(PIC2_DATA, offset2); io_wait(); // IRQ8..IRQ15 -> offset2 (обычно 0x28)
    
    // ICW3: Сообщаем Master PIC, что Slave PIC подключен к IRQ2
    outb(PIC1_DATA, 4); io_wait();
    // Сообщаем Slave PIC его каскадное ID
    outb(PIC2_DATA, 2); io_wait();
    
    // ICW4: Включаем режим 8086
    outb(PIC1_DATA, ICW4_8086); io_wait();
    outb(PIC2_DATA, ICW4_8086); io_wait();
    
    // Восстанавливаем сохраненные маски
    outb(PIC1_DATA, a1);
    outb(PIC2_DATA, a2);
}

} // namespace re36
