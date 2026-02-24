#include "kernel/pmm.h"
#include <stddef.h>
#include <stdint.h>

extern "C" {

// _sbrk - системный вызов, который запрашивает у ядра увеличение или уменьшение
// размера "кучи" данных (data segment). Стандартные функции malloc/new внутри 
// libc используют _sbrk для получения страниц памяти от ОС.
// Мы перенаправляем этот запрос в наш физический аллокатор (PMM).

void* _sbrk(int incr) {
    // В реальной ОС с виртуальной памятью (VMM/Paging) это сложнее.
    // Нам пришлось бы маппить физические адреса на виртуальные.
    // Но так как у нас пока только PMM, отдаем физические блоки.

    // Статическая переменная для хранения текущего "конца" данных
    // В настоящей ОС это адрес конца .bss секции, переданный линкером (extern int end;)
    // Для простоты выделим сразу с 0x500000 (5MB) - гарантированно за пределами ядра.
    static uint32_t heap_end = 0x500000; 

    uint32_t prev_heap_end = heap_end;

    // Если запросили уменьшить память - просто двигаем указатель (утечка в PMM не страшна для прототипа)
    // Если 0 - просто хотят узнать текущий конец
    if (incr == 0) {
        return (void*)heap_end;
    }

    if (incr < 0) {
        heap_end += incr;
        return (void*)prev_heap_end;
    }

    // Выделяем кадры у PMM. Так как они могут быть не непрерывными, 
    // в будущем нам ОЧЕНЬ ПОНАДОБИТСЯ VMM (Paging). 
    // Пока считаем что в самом начале RAM много непрерывной памяти.
    // На данном этапе _sbrk просто "откусывает" от заранее выделенного огромного куска.
    
    // ПРЕДУПРЕЖДЕНИЕ: Эта реализация _sbrk очень "грязная" и работает 
    // потому что мы забронировали всю память в PMM до 32MB. 
    // Более правильный путь - это сделать VMM (Virtual Memory Manager)
    
    // Симулируем выделение - просто двигаем указатель. Так как с 1 по 32мб 
    // всё помечено как "свободное" в PMM, мы можем просто использовать этот кусок для heap.
    // Если дойдет до границы - возвращаем ошибке.

    if (heap_end + incr > 32 * 1024 * 1024) {
        // Кончилась физическая память
        return (void*)-1; 
    }

    heap_end += incr;
    return (void*)prev_heap_end;
}

// Заглушки для компиляции базовой libsupc++/libc, если они потребуют
// стандартных POSIX функций (мы же bare-metal ОС, у нас нет файлов пока)

int _close(int file) { (void)file; return -1; }
int _fstat(int file, void *st) { (void)file; (void)st; return 0; }
int _isatty(int file) { (void)file; return 1; }
int _lseek(int file, int ptr, int dir) { (void)file; (void)ptr; (void)dir; return 0; }
int _read(int file, char *ptr, int len) { (void)file; (void)ptr; (void)len; return 0; }

// _write используется printf и std::cout! Мы перенаправим это в наш VGA буфер!
int _write(int file, char *ptr, int len) {
    if (file == 1 || file == 2) { // stdout || stderr
        volatile uint16_t* vga_buffer = (volatile uint16_t*)0xB8000;
        static int cursor_x = 0;
        static int cursor_y = 15; // Печатаем с 15 строки (чтобы не затереть BSOD и логи клавиатуры)

        for (int i = 0; i < len; i++) {
            if (ptr[i] == '\n') {
                cursor_y++;
                cursor_x = 0;
                continue;
            }
            vga_buffer[cursor_y * 80 + cursor_x] = (uint16_t(ptr[i]) | (0x0F << 8)); // Белый текст
            cursor_x++;
            
            if (cursor_x >= 80) {
                cursor_x = 0;
                cursor_y++;
            }
            if (cursor_y >= 25) {
                // В идеале - скроллинг экрана вверх. Пока просто сброс
                cursor_y = 15; 
            }
        }
        return len;
    }
    return -1;
}

void _exit(int status) {
    (void)status;
    while (1) {
        asm volatile("cli; hlt");
    }
}

int _getpid() { return 1; }
int _kill(int pid, int sig) { (void)pid; (void)sig; return -1; }

} // extern "C"
