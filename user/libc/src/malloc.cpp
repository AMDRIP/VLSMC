#include <malloc.h>
#include <string.h>
#include <sys/syscall.h>

#define ALIGNMENT 8
// Выравнивание размера до кратного ALIGNMENT
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1))
#define MAGIC_NUMBER 0xDEADC0DE

struct MemBlock {
    size_t size;         // Размер блока (включая заголовок)
    bool is_free;        // Флаг свободы
    uint32_t magic;      // Сигнатура для проверки целостности кучи
    MemBlock* next;      // Следующий блок в куче (не в свободном списке, а физически)
    MemBlock* prev;      // Предыдущий блок в куче
};

static MemBlock* head = nullptr;
static MemBlock* tail = nullptr;

// Функция для добавления нового блока памяти в конец кучи (через sbrk)
static MemBlock* request_space(MemBlock* last, size_t size) {
    // В VLSMC sys_sbrk работает через SYS_SBRK syscall
    void* request = (void*)syscall(SYS_SBRK, size);
    if ((long)request == -1) {
        return nullptr; // OOM
    }

    MemBlock* block = (MemBlock*)request;
    block->size = size;
    block->is_free = false;
    block->magic = MAGIC_NUMBER;
    block->next = nullptr;
    block->prev = last;

    if (last) {
        last->next = block;
    }

    return block;
}

// Найти первый подходящий свободный блок (First-Fit)
static MemBlock* find_free_block(MemBlock** last, size_t size) {
    MemBlock* current = head;
    while (current && !(current->is_free && current->size >= size)) {
        *last = current;
        current = current->next;
    }
    return current;
}

// Разбить слишком большой блок на два: нужного размера и остаток (свободный)
static void split_block(MemBlock* block, size_t size) {
    if (block->size > size + sizeof(MemBlock) + ALIGNMENT) {
        MemBlock* new_block = (MemBlock*)((uint8_t*)block + size);
        new_block->size = block->size - size;
        new_block->is_free = true;
        new_block->magic = MAGIC_NUMBER;
        
        new_block->next = block->next;
        new_block->prev = block;

        if (block->next) {
            block->next->prev = new_block;
        } else {
            tail = new_block; // Если мы разбили последний блок
        }

        block->next = new_block;
        block->size = size;
    }
}

extern "C" void* malloc(size_t size) {
    if (size == 0) return nullptr;

    MemBlock* block;
    size_t total_size = ALIGN(size + sizeof(MemBlock));

    if (!head) {
        // Первый вызов malloc
        block = request_space(nullptr, total_size);
        if (!block) return nullptr;
        head = block;
        tail = block;
    } else {
        MemBlock* last = head;
        block = find_free_block(&last, total_size);

        if (!block) {
            // Не нашли свободный блок, расширяем кучу
            block = request_space(last, total_size);
            if (!block) return nullptr;
            tail = block;
        } else {
            // Нашли свободный блок
            block->is_free = false;
            split_block(block, total_size);
        }
    }

    return (void*)(block + 1); // Возвращаем указатель за заголовком
}

extern "C" void free(void* ptr) {
    if (!ptr) return;

    MemBlock* block = (MemBlock*)ptr - 1;

    // Проверка на Heap Corruption
    if (block->magic != MAGIC_NUMBER) {
        // Паника в ring 3 (abort)
        syscall(SYS_EXIT, 1);
        while(1);
    }

    block->is_free = true;

    // Coalescing (слияние) со следующим свободным
    if (block->next && block->next->is_free) {
        block->size += block->next->size;
        block->next = block->next->next;
        if (block->next) {
            block->next->prev = block;
        } else {
            tail = block;
        }
    }

    // Coalescing (слияние) с предыдущим свободным
    if (block->prev && block->prev->is_free) {
        block->prev->size += block->size;
        block->prev->next = block->next;
        if (block->next) {
            block->next->prev = block->prev;
        } else {
            tail = block->prev;
        }
        // block сместился на prev
        block = block->prev;
    }

    // Если этот свободный кусок оказался в самом конце кучи, мы можем вернуть его системе (sbrk < 0)
    if (block == tail) {
        if (block->prev) {
            block->prev->next = nullptr;
            tail = block->prev;
        } else {
            head = nullptr;
            tail = nullptr;
        }

        // Возвращаем память через отрицательный инкремент в sys_sbrk
        syscall(SYS_SBRK, -(long)block->size);
    }
}

extern "C" void* calloc(size_t nmemb, size_t size) {
    size_t total = nmemb * size;
    void* ptr = malloc(total);
    if (ptr) {
        char* p = (char*)ptr;
        for (size_t i = 0; i < total; i++) p[i] = 0;
    }
    return ptr;
}

extern "C" void* realloc(void* ptr, size_t size) {
    if (!ptr) return malloc(size);
    if (size == 0) {
        free(ptr);
        return nullptr;
    }

    MemBlock* block = (MemBlock*)ptr - 1;
    if (block->magic != MAGIC_NUMBER) {
        return nullptr; // Corruption
    }

    size_t user_size = block->size - sizeof(MemBlock);
    
    // Если новый размер влезает в старый блок
    if (size <= user_size) {
        // Мы могли бы вызвать split_block, но для простоты просто возвращаем тот же указатель
        return ptr;
    }

    // Иначе аллоцируем новый
    void* new_ptr = malloc(size);
    if (!new_ptr) return nullptr;

    char* dst = (char*)new_ptr;
    char* src = (char*)ptr;
    for (size_t i = 0; i < user_size; i++) {
        dst[i] = src[i];
    }

    free(ptr);
    return new_ptr;
}
