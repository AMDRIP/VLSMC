# 🚀 VLSMC v0.1.0 — First Official Release

> **RAND Elecorner 36** — концептуальная 32-битная bare-metal операционная система архитектуры x86.

Это первый официальный релиз операционной системы, написанной с нуля на C++17 и x86 ассемблере. Система загружается с дискеты, переходит в Protected Mode и предоставляет полное пользовательское окружение Ring 3 с ELF-загрузчиком.

---

## 🏗️ Ядро

- **Двухстадийный загрузчик** (Stage 1 + Stage 2) с поддержкой FAT12/FAT16 и передачей `boot_info`
- **32-битное ядро** в Protected Mode (C++17, freestanding)
- **PMM** (Physical Memory Manager) — битовая карта для управления физическими фреймами
- **VMM** (Virtual Memory Manager) — полноценный Page Directory / Page Table с поддержкой user/kernel пространств
- **Многозадачность** — Round-Robin планировщик с поддержкой Ring 0 → Ring 3 переключения контекста
- **IDT / PIC** — обработка аппаратных прерываний (таймер, клавиатура)
- **Блокирующий IPC** — межпроцессное взаимодействие через очереди сообщений с блокировкой потоков
- **34 системных вызова** (int 0x80) — от `SYS_EXIT` до `SYS_FSIZE`
- **ELF Loader** — загрузка и исполнение пользовательских ELF-бинарников в Ring 3
- **Kernel Panic** — экран с полной диагностической информацией (регистры, стек, контекст)

## 💾 Файловая система

- **FAT16 драйвер** с полной поддержкой чтения, записи, удаления файлов и обхода FAT-цепочек
- **Файловый ввод-вывод** через системные вызовы `SYS_FOPEN`, `SYS_FREAD`, `SYS_FWRITE`, `SYS_FCLOSE`
- Таблица файловых дескрипторов ядра (до 16 одновременно открытых файлов)

## 🖥️ Драйверы

- **VGA** — текстовый режим 80×25 с пользовательским курсором
- **Клавиатура PS/2** — полный скан-код → символ с блокирующим [getchar()](file:///y:/DEV/VLSMC/kernel/src/libc.cpp#11-14)
- **ATA/IDE** — чтение секторов с жесткого диска
- **PCI** — сканирование шины и идентификация устройств (команда `pci`)
- **Таймер PIT** — системный таймер с функцией [sleep()](file:///y:/DEV/VLSMC/user/app_api.h#24-27)

## 📚 Стандартная библиотека C (User-Space Libc)

Полностью собственная реализация Libc для пользовательского пространства:

| Заголовок | Функции |
|-----------|---------|
| **[string.h](file:///y:/DEV/VLSMC/user/libc/include/string.h)** | [strlen](file:///y:/DEV/VLSMC/user/libc/src/string.cpp#116-138), [strcpy](file:///y:/DEV/VLSMC/user/libc/src/string.cpp#146-169), [strcmp](file:///y:/DEV/VLSMC/user/libc/src/string.cpp#211-236), [strchr](file:///y:/DEV/VLSMC/user/libc/src/string.cpp#258-267), [strrchr](file:///y:/DEV/VLSMC/user/libc/src/string.cpp#268-278), [strcat](file:///y:/DEV/VLSMC/user/libc/src/string.cpp#179-185), [strncat](file:///y:/DEV/VLSMC/user/libc/src/string.cpp#186-195), [strlcpy](file:///y:/DEV/VLSMC/user/libc/src/string.cpp#170-178), [strlcat](file:///y:/DEV/VLSMC/user/libc/src/string.cpp#196-210), [strncmp](file:///y:/DEV/VLSMC/user/libc/src/string.cpp#237-251), [memcpy](file:///y:/DEV/VLSMC/user/libc/src/string.cpp#9-33), [memset](file:///y:/DEV/VLSMC/user/libc/src/string.cpp#52-80), [memmove](file:///y:/DEV/VLSMC/user/libc/src/string.cpp#34-51), [memcmp](file:///y:/DEV/VLSMC/user/libc/src/string.cpp#81-109), [memcmp_s](file:///y:/DEV/VLSMC/user/libc/src/string.cpp#110-115) |
| **[stdio.h](file:///y:/DEV/VLSMC/user/libc/include/stdio.h)** | [printf](file:///y:/DEV/VLSMC/user/libc/src/stdio.cpp#146-223) (`%d`, `%s`, `%x`, `%p`, `%c`, **`%f`**), [putchar](file:///y:/DEV/VLSMC/user/libc/src/stdio.cpp#16-26), [puts](file:///y:/DEV/VLSMC/user/libc/src/stdio.cpp#27-36), [getchar](file:///y:/DEV/VLSMC/kernel/src/libc.cpp#11-14), [gets_s](file:///y:/DEV/VLSMC/user/libc/src/stdio.cpp#230-265), **[fopen](file:///y:/DEV/VLSMC/user/libc/src/stdio.cpp#266-286)**, **[fread](file:///y:/DEV/VLSMC/user/libc/src/stdio.cpp#294-304)**, **[fwrite](file:///y:/DEV/VLSMC/user/libc/src/stdio.cpp#305-315)**, **[fclose](file:///y:/DEV/VLSMC/user/libc/src/stdio.cpp#287-293)**, [feof](file:///y:/DEV/VLSMC/user/libc/src/stdio.cpp#316-320), [ferror](file:///y:/DEV/VLSMC/user/libc/src/stdio.cpp#321-325) |
| **[stdlib.h](file:///y:/DEV/VLSMC/user/libc/include/stdlib.h)** | [atoi](file:///y:/DEV/VLSMC/kernel/src/libc.cpp#48-68), [itoa](file:///y:/DEV/VLSMC/user/libc/src/stdlib.cpp#48-85), [abs](file:///y:/DEV/VLSMC/user/libc/src/stdlib.cpp#86-89), [labs](file:///y:/DEV/VLSMC/user/libc/src/stdlib.cpp#90-93), [div](file:///y:/DEV/VLSMC/user/libc/src/stdlib.cpp#94-105), [ldiv](file:///y:/DEV/VLSMC/user/libc/src/stdlib.cpp#106-117), [rand](file:///y:/DEV/VLSMC/user/libc/src/stdlib.cpp#118-122), [srand](file:///y:/DEV/VLSMC/user/libc/src/stdlib.cpp#123-126), [exit](file:///y:/DEV/VLSMC/user/libc/src/stdlib.cpp#6-10), [abort](file:///y:/DEV/VLSMC/user/libc/src/stdlib.cpp#11-14) |
| **[math.h](file:///y:/DEV/VLSMC/user/libc/include/math.h)** | [sqrt](file:///y:/DEV/VLSMC/user/libc/src/math.cpp#15-20), [pow](file:///y:/DEV/VLSMC/user/libc/src/math.cpp#51-72), [fabs](file:///y:/DEV/VLSMC/user/libc/src/math.cpp#3-8), [sin](file:///y:/DEV/VLSMC/user/libc/src/math.cpp#27-32), [cos](file:///y:/DEV/VLSMC/user/libc/src/math.cpp#39-44) + float-варианты ([sqrtf](file:///y:/DEV/VLSMC/user/libc/src/math.cpp#21-26), [powf](file:///y:/DEV/VLSMC/user/libc/src/math.cpp#73-76), [fabsf](file:///y:/DEV/VLSMC/user/libc/src/math.cpp#9-14), [sinf](file:///y:/DEV/VLSMC/user/libc/src/math.cpp#33-38), [cosf](file:///y:/DEV/VLSMC/user/libc/src/math.cpp#45-50)) |
| **[malloc.h](file:///y:/DEV/VLSMC/user/libc/include/malloc.h)** | [malloc](file:///y:/DEV/VLSMC/user/app_api.h#60-74), [free](file:///y:/DEV/VLSMC/user/app_api.h#75-88), [calloc](file:///y:/DEV/VLSMC/user/libc/src/malloc.cpp#170-178), [realloc](file:///y:/DEV/VLSMC/user/libc/src/malloc.cpp#179-202) |
| **[errno.h](file:///y:/DEV/VLSMC/user/libc/include/errno.h)** | Базовая поддержка кодов ошибок |

### ⚡ Оптимизации

- **SWAR (SIMD Within A Register)**: [strlen](file:///y:/DEV/VLSMC/user/libc/src/string.cpp#116-138), [strcpy](file:///y:/DEV/VLSMC/user/libc/src/string.cpp#146-169), [strcmp](file:///y:/DEV/VLSMC/user/libc/src/string.cpp#211-236), [memcpy](file:///y:/DEV/VLSMC/user/libc/src/string.cpp#9-33), [memset](file:///y:/DEV/VLSMC/user/libc/src/string.cpp#52-80) обрабатывают данные по 4 байта за такт (32-битные машинные слова)
- **Fast Bins**: Аллокации до 64 байт обслуживаются из специальных односвязных списков за O(1)
- **Boundary Tags**: Слияние свободных блоков памяти за O(1) без линейного обхода
- **x87 FPU**: Математические функции используют аппаратные инструкции сопроцессора (`fsqrt`, `fsin`, `fcos`, `fyl2x` + `f2xm1`)

## 📖 Документация

В папке `docs/` содержится обширная техническая документация:
- [ARCHITECTURE.md](file:///y:/DEV/VLSMC/docs/ARCHITECTURE.md) — Архитектура ядра
- [USER_APP_GUIDE.md](file:///y:/DEV/VLSMC/docs/USER_APP_GUIDE.md) — Руководство по созданию приложений
- [LIBC_REQUIREMENTS.md](file:///y:/DEV/VLSMC/docs/LIBC_REQUIREMENTS.md) — Спецификация стандартной библиотеки
- [FUTURE_VISION.md](file:///y:/DEV/VLSMC/docs/FUTURE_VISION.md) — Дорожная карта (GUI, сеть, микроядро)

## 🚀 Быстрый запуск

```bash
./build.sh
qemu-system-i386 -fda disk.img -hda data.img -boot a
```

После загрузки доступны команды шелла: `help`, `pci`, `bootinfo`, [ls](file:///y:/DEV/VLSMC/kernel/src/syscalls_posix.cpp#63-64), и запуск ELF-программ ([HELLO.ELF](file:///y:/DEV/VLSMC/HELLO.ELF), `FILETST.ELF`, `MATHTEST.ELF` и др.).

---

**Платформа**: x86 (IA-32) | **Инструменты**: NASM, GCC (cross-compiler), QEMU
