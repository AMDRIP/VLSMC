#include "kernel/kmalloc.h"
#include "kernel/pmm.h"

namespace re36 {

// Максимально примитивный аллокатор кучи (Heap) на базе связного списка.
// В реальности аллокаторы сложнее, но для вызова new и размещения пары классов этого хватит.

struct MemoryBlock {
    size_t size;           // Размер вместе со структурой (MemoryBlock)
    bool is_free;          // Свободен ли блок
    MemoryBlock* next;     // Следующий блок в MemoryBlockList
};

static MemoryBlock* heap_start = nullptr;

void kmalloc_init() {
    // Выделяем первый физический фрейм (4 КБ) в качестве основы для кучи
    heap_start = (MemoryBlock*)PhysicalMemoryManager::alloc_frame();
    if (!heap_start) return; // Упс, память кончилась на самом старте.

    // Инициализируем первый блок
    heap_start->size = PMM_FRAME_SIZE;
    heap_start->is_free = true;
    heap_start->next = nullptr;
}

// Простая функция разделения блока (если он сильно больше, чем просили)
static void split_block(MemoryBlock* block, size_t size) {
    if (block->size <= size + sizeof(MemoryBlock) + 32) {
        return; // Блок слишком маленький, чтобы его дробить
    }

    // Создаем новый блок после запрашиваемого размера
    MemoryBlock* new_block = (MemoryBlock*)((uint8_t*)block + size);
    new_block->size = block->size - size;
    new_block->is_free = true;
    new_block->next = block->next;

    block->size = size;
    block->next = new_block;
}

void* kmalloc(size_t size) {
    if (size == 0) return nullptr;

    // Выравниваем размер до 8 байт + добавляем заголовок
    size_t total_size = size + sizeof(MemoryBlock);
    if (total_size % 8 != 0) {
        total_size += 8 - (total_size % 8);
    }

    MemoryBlock* current = heap_start;
    MemoryBlock* last = nullptr;

    // Ищем свободный блок подходящего размера
    while (current) {
        if (current->is_free && current->size >= total_size) {
            current->is_free = false;
            split_block(current, total_size);
            return (void*)((uint8_t*)current + sizeof(MemoryBlock));
        }
        last = current;
        current = current->next;
    }

    // Если не нашли свободного места, мы должны попросить у PMM еще памяти.
    // Просим нужное количество фреймов и линкуем к куче
    size_t pages_needed = total_size / PMM_FRAME_SIZE;
    if (total_size % PMM_FRAME_SIZE != 0) pages_needed++;

    MemoryBlock* new_area = (MemoryBlock*)PhysicalMemoryManager::alloc_blocks(pages_needed);
    
    // В полноценной ОС здесь надо просить VMM замаппить вирт в физическую 
    // последовательно, а пока мы работаем с физической, и мы просим смежные фреймы у PMM.
    if (!new_area) return nullptr;

    new_area->size = pages_needed * PMM_FRAME_SIZE;
    new_area->is_free = false; // Сразу забираем под нашу аллокацию
    new_area->next = nullptr;
    
    if (last) last->next = new_area;

    split_block(new_area, total_size);

    return (void*)((uint8_t*)new_area + sizeof(MemoryBlock));
}

void kfree(void* ptr) {
    if (!ptr) return;

    MemoryBlock* block = (MemoryBlock*)((uint8_t*)ptr - sizeof(MemoryBlock));
    block->is_free = true;

    // Простейшая дефрагментация: сливаем этот кусок со следующим, если он свободен
    if (block->next && block->next->is_free) {
        block->size += block->next->size;
        block->next = block->next->next;
    }
}

} // namespace re36

// --- Реализация глобальных операторов new / delete ---

void* operator new(size_t size) {
    return re36::kmalloc(size);
}

void* operator new[](size_t size) {
    return re36::kmalloc(size);
}

void operator delete(void* p) {
    re36::kfree(p);
}

void operator delete(void* p, size_t size) {
    (void)size;
    re36::kfree(p);
}

void operator delete[](void* p) {
    re36::kfree(p);
}

void operator delete[](void* p, size_t size) {
    (void)size;
    re36::kfree(p);
}

// Заглушка для чисто виртуальных функций, вызывается при краше абстрактных интерфейсов
extern "C" void __cxa_pure_virtual() {
    // В идеале выкинуть Kernel Panic, но пока оставим пустой или бесконечный цикл
    while (1);
}
