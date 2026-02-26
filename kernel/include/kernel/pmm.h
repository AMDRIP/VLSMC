#pragma once

#include <stdint.h>
#include <stddef.h>

namespace re36 {

// Размер одного фрейма (страницы) физической памяти - 4 Килобайта
#define PMM_FRAME_SIZE 4096

// Манипуляция битами в Bitmap
#define PMM_BITMAP_INDEX(a) (a / 32)
#define PMM_BITMAP_OFFSET(a) (a % 32)

class PhysicalMemoryManager {
public:
    // Инициализация PMM. 
    // bitmap_addr - физический адрес, где будет лежать сам битмап (массив).
    // memory_size - общий размер доступной ОЗУ в байтах (напр. 32 МБ).
    static void init(uint32_t bitmap_addr, uint32_t memory_size);

    // Помечает регион памяти (size байт) как занятый или свободный
    static void set_region_free(uint32_t base, uint32_t size);
    static void set_region_used(uint32_t base, uint32_t size);

    // Выделяет первый попавшийся свободный фрейм (4 КБ) и возвращает его физический адрес
    static void* alloc_frame();
    
    // Выделяет непрерывный блок из count фреймов и возвращает физический адрес
    static void* alloc_blocks(uint32_t count);
    
    // Освобождает фрейм по физическому адресу
    static void free_frame(void* frame_addr);

    // Возвращает кол-во свободной и занятой памяти (для логов/статистики)
    static uint32_t get_free_memory();
    static uint32_t get_used_memory();

private:
    // Установить / Сбросить бит (занять/освободить фрейм)
    static inline void set_frame(uint32_t frame);
    static inline void clear_frame(uint32_t frame);
    
    // Проверить, занят ли бит
    static inline bool test_frame(uint32_t frame);

    // Найти первый свободный фрейм (index)
    static uint32_t get_first_free_frame();
    
    // Найти последовательность из count свободных фреймов
    static uint32_t get_free_blocks(uint32_t count);

private:
    static uint32_t* memory_bitmap_; // Указатель на массив битмапа
    static uint32_t max_frames_;     // Сколько всего фреймов (memory_size / 4096)
    static uint32_t used_frames_;    // Сколько фреймов занято сейчас
};

} // namespace re36
