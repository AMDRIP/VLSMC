# Требования к LIBC.SO — динамической стандартной библиотеке C для VLSMC

## 1. Обзор

`libc.so` — разделяемая библиотека (shared object, ELF ET_DYN), предоставляющая стандартный интерфейс C для пользовательских приложений RAND Elecorner 36. Она загружается динамическим линковщиком (`ldso`) в адресное пространство процесса при запуске ELF-приложения.

### Текущее состояние

Libc существует как **статическая** библиотека из 8 модулей:

| Модуль | Файл | Статус |
|--------|-------|--------|
| string | `string.cpp` | Полный (SWAR-оптимизация) |
| malloc | `malloc.cpp` | Рабочий (First-Fit, Boundary Tags, Coalescing) |
| stdio | `stdio.cpp` | Рабочий (printf, getchar, gets_s, fopen/fread/fwrite/fclose, буферизация stdout) |
| stdlib | `stdlib.cpp` | Рабочий (atoi, itoa, abs, div, rand, fork, execve, wait) |
| math | `math.cpp` | Базовый (целочисленные и FPU операции) |
| errno | `errno.cpp` | Глобальная переменная errno |
| cxx | `cxx.cpp` | __cxa_pure_virtual |
| syscall | `syscall.cpp` | Variadic обёртка syscall(number, ...) |

### Цель

Превратить статическую libc в `libc.so` (ET_DYN), которую ldso загружает по фиксированному адресу в адресное пространство каждого процесса.

---

## 2. Архитектурные требования

### 2.1. Формат и ABI

- **Формат**: ELF32 ET_DYN (i386), Position-Independent Code (PIC) через `-fPIC`
- **ABI вызовов**: cdecl (стандарт для i386 bare-metal)
- **Соглашение имён**: все публичные символы — `extern "C"`, без C++ name mangling
- **Выравнивание**: данные — 4 байта, malloc — 8 байт (16 для SSE-совместимости в будущем)

### 2.2. Компиляция

```
i686-elf-g++ -fPIC -shared -nostdlib -ffreestanding \
  -fno-exceptions -fno-rtti \
  -o libc.so \
  string.cpp malloc.cpp stdio.cpp stdlib.cpp math.cpp \
  errno.cpp cxx.cpp syscall.cpp \
  -T libc.ld
```

Флаги:
- `-fPIC` — Position-Independent Code (обязательно для .so)
- `-shared` — ET_DYN вместо ET_EXEC
- `-nostdlib -ffreestanding` — без системного CRT и libc хоста
- `-fno-exceptions -fno-rtti` — убираем C++ overhead

### 2.3. Линкер-скрипт (libc.ld)

```
OUTPUT_FORMAT(elf32-i386)

SECTIONS
{
    . = SIZEOF_HEADERS;

    .text : ALIGN(4K) {
        *(.text*)
    }

    .rodata : ALIGN(4) {
        *(.rodata*)
    }

    .data.rel.ro : ALIGN(4) {
        *(.data.rel.ro*)
    }

    .dynamic : {
        *(.dynamic)
    }

    .dynsym : {
        *(.dynsym)
    }

    .dynstr : {
        *(.dynstr)
    }

    .hash : {
        *(.hash)
    }

    .got : {
        *(.got*)
    }

    .rel.dyn : {
        *(.rel.dyn)
        *(.rel.plt)
    }

    .data : ALIGN(4K) {
        *(.data*)
    }

    .bss : ALIGN(4K) {
        *(.bss*)
        *(COMMON)
    }
}
```

> [!IMPORTANT]
> Секции `.dynsym`, `.dynstr`, `.hash` **обязательны** — ldso ищет символы через них. Без `.hash` разрешение символов невозможно.

### 2.4. Адресация загрузки

ldso загружает `libc.so` начиная с адреса `0x60000000` (текущее значение `g_next_load_addr` в ldso). Каждый следующий `.so` смещается на `0x01000000`. PIC-код через GOT/PLT обеспечивает корректную работу при любом базовом адресе.

---

## 3. Экспортируемые символы

### 3.1. Обязательный минимум (Tier 1)

Символы, без которых ни одно приложение не запустится:

| Категория | Символы |
|-----------|---------|
| **Память** | `malloc`, `free`, `calloc`, `realloc` |
| **Строки** | `memcpy`, `memmove`, `memset`, `memcmp`, `strlen`, `strcpy`, `strcmp`, `strncmp`, `strchr`, `strstr`, `strcat` |
| **I/O** | `printf`, `putchar`, `puts`, `getchar`, `gets_s` |
| **Файлы** | `fopen`, `fclose`, `fread`, `fwrite`, `fgetc`, `fputc`, `fflush`, `feof`, `ferror` |
| **Системные** | `syscall`, `exit`, `abort` |
| **C++ поддержка** | `operator new`, `operator new[]`, `operator delete`, `operator delete[]`, `__cxa_pure_virtual` |
| **errno** | `errno` (глобальная переменная) |

### 3.2. Расширенный набор (Tier 2)

Символы, необходимые для полноценных приложений:

| Категория | Символы |
|-----------|---------|
| **Безопасные строки** | `strlcpy`, `strlcat`, `strnlen`, `strncmp_s`, `memcmp_s`, `strncat`, `strrchr` |
| **Процессы** | `fork`, `execve`, `exec`, `wait` |
| **Конвертация** | `atoi`, `itoa` |
| **Математика** | `abs`, `labs`, `div`, `ldiv`, `rand`, `srand` |
| **Float I/O** | Поддержка `%f` в printf |

### 3.3. Visibility

Все публичные функции помечаются `__attribute__((visibility("default")))`. Внутренние helper-функции (`request_space`, `find_free_block`, `split_block`, `reverse_string`, `print_uint_core` и т.д.) компилируются с `__attribute__((visibility("hidden")))` или объявляются `static`.

---

## 4. Требования к модулям

### 4.1. syscall.cpp — Фундамент

Единственный модуль, содержащий inline asm для `int 0x80`. Все остальные модули libc обращаются к ядру **только** через `syscall()`.

**Требования:**
- Variadic `syscall(long number, ...)` — до 5 аргументов через EAX, EBX, ECX, EDX, ESI, EDI
- `errno` устанавливается при ошибке (ret < 0 && ret > -4096)
- Inline-обёртки `__syscall0..5` в заголовке `sys/syscall.h` — для hot-path (избежать variadic overhead)

### 4.2. malloc.cpp — Аллокатор

**Критические требования для .so:**
- Глобальные переменные `head`, `tail` — должны быть в `.bss` секции libc.so
- `sbrk` вызывается через `syscall(SYS_SBRK, ...)` — адреса кучи уникальны для каждого процесса
- `operator new`/`operator delete` экспортируются для C++ приложений

> [!CAUTION]
> При динамической загрузке libc.so, **каждый процесс** получает свою копию `.data`/`.bss` секций (CoW при fork). Heap-состояние (`head`/`tail`) не shared между процессами.

### 4.3. stdio.cpp — Буферизованный I/O

**Требования:**
- `stdout_buf` / `stdout_pos` — per-process (в `.bss`)
- `fflush` при `\n` (line buffering) или при переполнении буфера
- FILE-структуры выделяются через `malloc` из libc.so
- `printf` должен поддерживать: `%d`, `%u`, `%x`, `%X`, `%s`, `%c`, `%p`, `%f`, `%%`
- Ширина полей и padding: `%08x`, `%-20s`, `%5d`

### 4.4. string.cpp — Строковые операции

- SWAR-оптимизация для `memcpy`, `memset`, `memcmp`, `strlen`, `strcpy`, `strcmp` — уже реализована
- PIC-совместимость: SWAR-макросы не зависят от абсолютных адресов — OK

### 4.5. cxx.cpp — C++ Runtime поддержка

- `__cxa_pure_virtual` — вызывает `printf` + `exit(1)`
- `__cxa_atexit` — заглушка (возврат 0), необходим для глобальных деструкторов
- `__dso_handle` — слабый символ, нужен для `__cxa_atexit`

---

## 5. Интеграция с ldso

### 5.1. Загрузка libc.so

Сценарий загрузки:

```
1. Ядро запускает ELF (sys_exec)
2. ELF содержит PT_INTERP = "/lib/ld.so"  (или NEEDED = "libc.so")
3. ldso (_ld_main) сканирует DT_NEEDED записи приложения
4. ldso вызывает load_shared_object("libc.so")
5. ldso открывает /lib/libc.so через sys_fopen
6. ldso читает ELF-заголовок, маппит PT_LOAD сегменты через sys_mmap
7. ldso парсит .dynamic секцию libc.so
8. ldso вызывает register_object() — регистрирует symtab/strtab
9. ldso обрабатывает R_386_32 и R_386_RELATIVE релокации
10. ldso разрешает символы приложения через lookup_symbol_global()
11. ldso передаёт управление в entry point приложения
```

### 5.2. Поддерживаемые релокации

ldso на данный момент обрабатывает:

| Тип | Описание | Формула |
|-----|----------|---------|
| `R_386_32` (1) | Абсолютный адрес символа | `S + A` |
| `R_386_RELATIVE` (8) | Относительный (PIC) | `B + A` |
| `R_386_GLOB_DAT` (6) | GOT запись | `S` |
| `R_386_JMP_SLOT` (7) | PLT запись | `S` |

> [!NOTE]
> `R_386_PC32` (2) — PC-relative — пока **не поддерживается** в ldso. Если GCC генерирует такие релокации для `-fPIC`, нужно добавить обработку в `process_relocations()`.

### 5.3. Размещение на диске

```
/lib/libc.so     — основная библиотека
/lib/ld.so       — динамический линковщик
```

Файлы записываются на FAT12 образ во время сборки.

---

## 6. Требования к errno и TLS

### Текущая реализация
`errno` — простая глобальная переменная в `.bss`. Каждый процесс получает свою копию при fork (CoW).

### Будущее (при многопоточности)
- `errno` должна стать TLS-переменной (`__thread int errno`)
- Требует поддержки TLS в ядре (GDT-сегмент для TLS, загрузка `%gs` при переключении контекста)
- Libc должна вызывать `syscall(SYS_SET_TLS, base)` для установки TLS-блока

> [!WARNING]
> До реализации TLS в ядре, **errno НЕ thread-safe**. Многопоточные приложения не должны полагаться на errno.

---

## 7. Порядок реализации

### Этап 1: Минимальная libc.so (MVP)
1. Создать `user/libc/libc.ld` — линкер-скрипт для shared object
2. Добавить `-fPIC` к компиляции всех `.cpp` файлов libc
3. Компоновать `libc.so` через `-shared -nostdlib`
4. Добавить `__cxa_atexit` заглушку и `__dso_handle`
5. Записать `libc.so` в `/lib/` на FAT12 образ
6. Протестировать загрузку через ldso

### Этап 2: Тестирование
1. Скомпилировать `hello.cpp` как динамически линкуемый ELF (`-dynamic-linker /lib/ld.so -lc`)
2. Запустить в QEMU
3. Проверить: printf работает, malloc работает, exit завершает процесс

### Этап 3: Расширение
1. Добавить недостающие POSIX-совместимые символы по мере необходимости
2. Оптимизация загрузки (Lazy Binding через PLT/GOT)
3. TLS для errno

---

## 8. Стиль кода

В соответствии с глобальными правилами проекта:
- **Запрещены** все комментарии в исходном коде
- Все функции `extern "C"` 
- Внутренние функции `static`
- Никакого C++ name mangling в публичных символах
