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
    static void init(uint32_t bitmap_addr, uint32_t memory_size, uint32_t direct_map_limit);
    static uint32_t calculate_metadata_size(uint32_t memory_size);

    // Помечает регион памяти (size байт) как занятый или свободный
    static void set_region_free(uint32_t base, uint32_t size);
    static void set_region_used(uint32_t base, uint32_t size);

    // Выделяет первый попавшийся свободный фрейм (4 КБ) и возвращает его физический адрес
    static void* alloc_frame();
    static void* alloc_high_frame();
    static void* alloc_user_frame();
    static void* alloc_frame_any();
    
    // Выделяет непрерывный блок из count фреймов и возвращает физический адрес
    static void* alloc_blocks(uint32_t count);
    
    // Освобождает фрейм по физическому адресу
    static void free_frame(void* frame_addr);

    static void inc_ref(uint32_t phys_addr);
    static void dec_ref(uint32_t phys_addr);
    static uint8_t get_refcount(uint32_t phys_addr);

    static uint32_t get_free_memory();
    static uint32_t get_used_memory();
    static uint32_t get_total_memory();
    static uint32_t get_managed_memory_limit();
    static uint32_t get_direct_map_limit();
    static uint32_t get_direct_mapped_memory();
    static uint32_t get_high_memory();
    static uint32_t get_free_direct_mapped_memory();
    static uint32_t get_free_high_memory();
    static bool is_direct_mapped(uint32_t phys_addr);

private:
    // Установить / Сбросить бит (занять/освободить фрейм)
    static inline void set_frame(uint32_t frame);
    static inline void clear_frame(uint32_t frame);
    
    // Проверить, занят ли бит
    static inline bool test_frame(uint32_t frame);

    // Найти первый свободный фрейм (index)
    static uint32_t get_first_free_frame(uint32_t start_frame, uint32_t end_frame);
    
    // Найти последовательность из count свободных фреймов
    static uint32_t get_free_blocks(uint32_t count, uint32_t start_frame, uint32_t end_frame);
    static uint32_t count_free_frames(uint32_t start_frame, uint32_t end_frame);

private:
    static uint32_t* memory_bitmap_;
    static uint32_t max_frames_;
    static uint32_t used_frames_;
    static uint32_t usable_frames_;
    static uint32_t direct_usable_frames_;
    static uint32_t high_usable_frames_;
    static uint32_t managed_memory_limit_;
    static uint32_t direct_map_limit_;
    static uint8_t* refcounts_;
};

} // namespace re36
