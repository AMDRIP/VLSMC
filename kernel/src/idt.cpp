#include "kernel/idt.h"
#include "kernel/pic.h"
#include "kernel/keyboard.h"
#include "kernel/timer.h"
#include "kernel/task_scheduler.h"
#include "kernel/vmm.h"
#include "kernel/syscall_gate.h"
#include "libc.h"

namespace re36 {

#define IDT_ENTRIES 256

IdtEntry idt[IDT_ENTRIES];
IdtPtr idt_ptr;

// Внешние обработчики (ISRs) из Assembly
extern "C" {
    void isr0();
    void isr1();
    void isr2();
    void isr3();
    void isr4();
    void isr5();
    void isr6();
    void isr7();
    void isr8();
    void isr9();
    void isr10();
    void isr11();
    void isr12();
    void isr13();
    void isr14();
    void isr15();
    void isr16();
    void isr17();
    void isr18();
    void isr19();
    void isr20();
    void isr21();
    void isr22();
    void isr23();
    void isr24();
    void isr25();
    void isr26();
    void isr27();
    void isr28();
    void isr29();
    void isr30();
    void isr31();
    
    // IRQs (Аппаратные)
    void irq0();
    void irq1();
    void irq2();
    void irq3();
    void irq4();
    void irq5();
    void irq6();
    void irq7();
    void irq8();
    void irq9();
    void irq10();
    void irq11();
    void irq12();
    void irq13();
    void irq14();
    void irq15();

    // Функция загрузки IDT (написана на Assembly)
    void load_idt(uint32_t idt_ptr);
}

void set_idt_gate(int n, uint32_t handler, uint16_t selector, uint8_t flags) {
    idt[n].isr_low = handler & 0xFFFF;
    idt[n].kernel_cs = selector;
    idt[n].reserved = 0;
    idt[n].attributes = flags;
    idt[n].isr_high = (handler >> 16) & 0xFFFF;
}

void init_idt() {
    idt_ptr.base = (uint32_t)&idt;
    idt_ptr.limit = IDT_ENTRIES * sizeof(IdtEntry) - 1;

    // Исключения CPU (0-31)
    set_idt_gate(0, (uint32_t)isr0, 0x08, 0x8E);
    set_idt_gate(1, (uint32_t)isr1, 0x08, 0x8E);
    set_idt_gate(2, (uint32_t)isr2, 0x08, 0x8E);
    set_idt_gate(3, (uint32_t)isr3, 0x08, 0x8E);
    set_idt_gate(4, (uint32_t)isr4, 0x08, 0x8E);
    set_idt_gate(5, (uint32_t)isr5, 0x08, 0x8E);
    set_idt_gate(6, (uint32_t)isr6, 0x08, 0x8E);
    set_idt_gate(7, (uint32_t)isr7, 0x08, 0x8E);
    set_idt_gate(8, (uint32_t)isr8, 0x08, 0x8E);
    set_idt_gate(9, (uint32_t)isr9, 0x08, 0x8E);
    set_idt_gate(10, (uint32_t)isr10, 0x08, 0x8E);
    set_idt_gate(11, (uint32_t)isr11, 0x08, 0x8E);
    set_idt_gate(12, (uint32_t)isr12, 0x08, 0x8E);
    set_idt_gate(13, (uint32_t)isr13, 0x08, 0x8E);
    set_idt_gate(14, (uint32_t)isr14, 0x08, 0x8E);
    set_idt_gate(15, (uint32_t)isr15, 0x08, 0x8E);
    set_idt_gate(16, (uint32_t)isr16, 0x08, 0x8E);
    set_idt_gate(17, (uint32_t)isr17, 0x08, 0x8E);
    set_idt_gate(18, (uint32_t)isr18, 0x08, 0x8E);
    set_idt_gate(19, (uint32_t)isr19, 0x08, 0x8E);
    set_idt_gate(20, (uint32_t)isr20, 0x08, 0x8E);
    set_idt_gate(21, (uint32_t)isr21, 0x08, 0x8E);
    set_idt_gate(22, (uint32_t)isr22, 0x08, 0x8E);
    set_idt_gate(23, (uint32_t)isr23, 0x08, 0x8E);
    set_idt_gate(24, (uint32_t)isr24, 0x08, 0x8E);
    set_idt_gate(25, (uint32_t)isr25, 0x08, 0x8E);
    set_idt_gate(26, (uint32_t)isr26, 0x08, 0x8E);
    set_idt_gate(27, (uint32_t)isr27, 0x08, 0x8E);
    set_idt_gate(28, (uint32_t)isr28, 0x08, 0x8E);
    set_idt_gate(29, (uint32_t)isr29, 0x08, 0x8E);
    set_idt_gate(30, (uint32_t)isr30, 0x08, 0x8E);
    set_idt_gate(31, (uint32_t)isr31, 0x08, 0x8E);

    // Аппаратные прерывания PIC (32-47)
    set_idt_gate(32, (uint32_t)irq0, 0x08, 0x8E);
    set_idt_gate(33, (uint32_t)irq1, 0x08, 0x8E);
    set_idt_gate(34, (uint32_t)irq2, 0x08, 0x8E);
    set_idt_gate(35, (uint32_t)irq3, 0x08, 0x8E);
    set_idt_gate(36, (uint32_t)irq4, 0x08, 0x8E);
    set_idt_gate(37, (uint32_t)irq5, 0x08, 0x8E);
    set_idt_gate(38, (uint32_t)irq6, 0x08, 0x8E);
    set_idt_gate(39, (uint32_t)irq7, 0x08, 0x8E);
    set_idt_gate(40, (uint32_t)irq8, 0x08, 0x8E);
    set_idt_gate(41, (uint32_t)irq9, 0x08, 0x8E);
    set_idt_gate(42, (uint32_t)irq10, 0x08, 0x8E);
    set_idt_gate(43, (uint32_t)irq11, 0x08, 0x8E);
    set_idt_gate(44, (uint32_t)irq12, 0x08, 0x8E);
    set_idt_gate(45, (uint32_t)irq13, 0x08, 0x8E);
    set_idt_gate(46, (uint32_t)irq14, 0x08, 0x8E);
    set_idt_gate(47, (uint32_t)irq15, 0x08, 0x8E);

    load_idt((uint32_t)&idt_ptr);
}

} // namespace re36

// Глобальный обработчик прерываний, вызываемый из Assembly.
// Должен быть снаружи namespace re36, но использовать его типы.
extern "C" void isr_handler(re36::Registers* regs) {
    if (regs->int_no >= 32 && regs->int_no <= 47) {

        if (regs->int_no == 32) {
            // IRQ0 - Таймер PIT
            re36::Timer::tick();
            re36::pic_send_eoi(0);
            re36::TaskScheduler::schedule();
            return;
        }

        if (regs->int_no == 33) {
            re36::KeyboardDriver::handle_interrupt();
        }

        re36::pic_send_eoi(regs->int_no - 32);
        
        return;
    }

    if (regs->int_no == 14) {
        uint32_t fault_addr;
        asm volatile("mov %%cr2, %0" : "=r"(fault_addr));

        if ((regs->cs & 3) == 3) {
            printf("\n[ESR] User Thread %d: Page Fault at 0x%x (%s %s)\n",
                re36::TaskScheduler::get_current_tid(),
                fault_addr,
                (regs->err_code & 0x1) ? "Protection" : "Not-Present",
                (regs->err_code & 0x2) ? "Write" : "Read");
            re36::TaskScheduler::terminate_current();
            return;
        }

        re36::VMM::handle_page_fault(fault_addr, regs->err_code);
        return;
    }

    if (regs->int_no == 128) {
        re36::SyscallRegs sregs;
        sregs.eax = regs->eax;
        sregs.ebx = regs->ebx;
        sregs.ecx = regs->ecx;
        sregs.edx = regs->edx;
        sregs.esi = regs->esi;
        sregs.edi = regs->edi;
        regs->eax = re36::handle_syscall(&sregs);
        return;
    }

    // Если это исключение CPU (номер 0-31)
    
    // Массив названий стандартных исключений x86
    const char* exception_messages[] = {
        "Division By Zero", "Debug", "Non Maskable Interrupt", "Breakpoint",
        "Into Detected Overflow", "Out of Bounds", "Invalid Opcode", "No Coprocessor",
        "Double Fault", "Coprocessor Segment Overrun", "Bad TSS", "Segment Not Present",
        "Stack Fault", "General Protection Fault", "Page Fault", "Unknown Interrupt",
        "Coprocessor Fault", "Alignment Check", "Machine Check", "SIMD Floating-Point",
        "Virtualization", "Control Protection", "Reserved", "Reserved",
        "Reserved", "Reserved", "Reserved", "Reserved",
        "Hypervisor Injection", "VMM Communication", "Security Exception", "Reserved"
    };

    // Защита (ESR Security): Если исключение произошло в пользовательском коде (Ring 3),
    // мы не должны крашить всю ОС. Просто убиваем упавший поток.
    if ((regs->cs & 3) == 3) {
        printf("\n[ESR] User Thread %d crashed!\n", re36::TaskScheduler::get_current_tid());
        const char* msg = (regs->int_no < 32) ? exception_messages[regs->int_no] : "Unknown";
        printf("[ESR] Exception: %s, EIP: 0x%x, ERR: 0x%x\n", msg, regs->eip, regs->err_code);
        re36::TaskScheduler::terminate_current();
        return;
    }

    volatile uint16_t* vga_buffer = (volatile uint16_t*)0xB8000;

    // Очистим экран красным цветом (Критическая ошибка ядра CPL=0)
    for (int i = 0; i < 80 * 25; i++) {
        vga_buffer[i] = (uint16_t(' ') | (0x4F << 8)); // 4 = Красный фон, F = Белый текст
    }
    
    // Вспомогательная лямбда для печати строки
    auto print_str = [&](int x, int y, const char* str) {
        int idx = y * 80 + x;
        for (int i = 0; str[i] != '\0' && i < 80; i++) { // Защита от бесконечного цикла
            if (idx >= 80 * 25) break; 
            vga_buffer[idx++] = (uint16_t(str[i]) | (0x4F << 8));
        }
    };

    // Вспомогательная лямбда для печати 32-битного HEX
    const char hex_chars[] = "0123456789ABCDEF";
    auto print_hex = [&](int x, int y, uint32_t val) {
        int idx = y * 80 + x;
        vga_buffer[idx++] = (uint16_t('0') | (0x4F << 8));
        vga_buffer[idx++] = (uint16_t('x') | (0x4F << 8));
        for (int i = 0; i < 8; i++) {
            uint8_t nibble = (val >> (28 - i * 4)) & 0x0F;
            vga_buffer[idx++] = (uint16_t(hex_chars[nibble]) | (0x4F << 8));
        }
    };

    print_str(0, 0, "================================================================================");
    print_str(0, 1, "                            KERNEL PANIC: EXCEPTION                             ");
    print_str(0, 2, "================================================================================");

    // Выводим название исключения
    if (regs->int_no < 32) {
        print_str(2, 4, "Exception: ");
        print_str(13, 4, exception_messages[regs->int_no]);
    } else {
        print_str(2, 4, "Exception: Unknown Software Interrupt");
    }

    print_str(2, 5, "Error Code:");
    print_hex(14, 5, regs->err_code);

    // Выводим дамп регистров
    print_str(2, 7,  "EAX:"); print_hex(7, 7, regs->eax);
    print_str(22, 7, "EBX:"); print_hex(27, 7, regs->ebx);
    print_str(42, 7, "ECX:"); print_hex(47, 7, regs->ecx);
    print_str(62, 7, "EDX:"); print_hex(67, 7, regs->edx);

    print_str(2, 8,  "ESI:"); print_hex(7, 8, regs->esi);
    print_str(22, 8, "EDI:"); print_hex(27, 8, regs->edi);
    print_str(42, 8, "EBP:"); print_hex(47, 8, regs->ebp);
    print_str(62, 8, "ESP:"); print_hex(67, 8, regs->esp);

    print_str(2, 10,  "EIP:"); print_hex(7, 10, regs->eip);
    print_str(22, 10, "CS: "); print_hex(27, 10, regs->cs);
    print_str(42, 10, "DS: "); print_hex(47, 10, regs->ds);
    print_str(62, 10, "EFL:"); print_hex(67, 10, regs->eflags);

    print_str(0, 13, "SYSTEM HALTED.");

    // Бесконечный цикл остановки процессора
    while (true) {
        asm volatile("cli; hlt");
    }
}
