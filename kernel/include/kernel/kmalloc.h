#pragma once

#include <stdint.h>
#include <stddef.h>

namespace re36 {

// Инициализация кучи (heap) ядра.
void kmalloc_init();

// Выделить блок памяти заданного размера (в байтах)
void* kmalloc(size_t size);

// Освободить ранее выделенный блок
void kfree(void* ptr);

} // namespace re36

// --- Глобальные C++ операторы (требуются для работы классов, vector, string и т.д.) ---
// Так как мы находимся в среде -ffreestanding (без libstdc++), мы должны определить их сами.
void* operator new(size_t size);
void* operator new[](size_t size);
void operator delete(void* p);
void operator delete(void* p, size_t size);
void operator delete[](void* p);
void operator delete[](void* p, size_t size);
