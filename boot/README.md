# RAND Elecorner 36 — Двухстадийный загрузчик

## Архитектура

Загрузчик состоит из двух стадий, что позволяет обойти ограничение boot-сектора в 512 байт.

```
BIOS ──► Stage 1 (512B, сектор 0) ──► Stage 2 (STAGE2.BIN) ──► KERNEL.BIN
         bootloader.asm                stage2.asm                kernel_entry.asm
         0x7C00                        0x8000                    0x10000
```

## Stage 1 — `bootloader.asm` (512 байт)

Минимальный код, умещающийся в boot-сектор:

| Шаг | Описание |
|-----|----------|
| 1 | Настройка сегментов: `DS=ES=SS=0`, `SP=0x7C00` |
| 2 | Сохранение `DL` (boot drive от BIOS) |
| 3 | Вычисление LBA корневого каталога из BPB |
| 4 | Чтение корневого каталога → `0x7E00` |
| 5 | Поиск файла `STAGE2.BIN` (8.3 формат) |
| 6 | Вычисление LBA первого кластера |
| 7 | Загрузка 8 секторов (4 КБ) → `0x8000` |
| 8 | Передача `DL` и `jmp 0x0000:0x8000` |

**LBA→CHS конвертация** через `INT 13h AH=02h` — дискеты не поддерживают Extended Read (AH=42h).

## Stage 2 — `stage2.asm` (~1 КБ)

Полноценный загрузчик без ограничений по размеру:

| Шаг | Описание |
|-----|----------|
| 1 | Стек в безопасной зоне: `SS=0`, `SP=0x7C00` |
| 2 | Чтение FAT таблицы (9 секторов) → `0x2000` |
| 3 | Поиск `KERNEL.BIN` в корневом каталоге |
| 4 | Валидация первого кластера (`2 ≤ cluster < 0xFF0`) |
| 5 | Следование **FAT12 цепочке кластеров** (12-бит) |
| 6 | Валидация каждого LBA (`< TotalSectors`) |
| 7 | Загрузка ядра → `0x1000:0x0000` (физ. `0x10000`) |
| 8 | Детекция RAM: `INT 15h AH=E801h`, fallback `AH=88h` |
| 9 | Заполнение `BootInfo` структуры → `0x0500` |
| 10 | Проверка magic `0xAA55` в boot-секторе |
| 11 | `jmp 0x1000:0x0000` → передача управления ядру |

### FAT12 парсинг кластеров

```
Если cluster чётный:  next = FAT[cluster*3/2] & 0x0FFF
Если cluster нечётный: next = FAT[cluster*3/2] >> 4
Конец цепочки:         next >= 0x0FF8
```

### Диагностика

Stage 2 выводит подробный лог на каждом шаге:

```
S1.S2.
[Stage2] Init
[Stage2] Stack=0:7C00 FAT=0x2000
[Stage2] Reading FAT (9 sec)...
[Stage2] FAT loaded OK
[Stage2] Searching KERNEL.BIN...
[Stage2] First cluster: 5
[Stage2] Loading kernel (FAT12 chain)...
[Stage2] Loaded 97 sectors
[Stage2] RAM: 32768 KB
[Stage2] Boot OK! -> 0x10000
```

При ошибках — чёткие сообщения: `Bad cluster!`, `LBA out of range!`, `Chain corrupt!`, `Disk read!`.

## Карта памяти (Real Mode)

```
0x0000─0x04FF  IVT + BDA (BIOS)
0x0500─0x050F  BootInfo struct
0x0510─0x1FFF  ── свободно (стек растёт сюда ▼) ──
0x2000─0x31FF  FAT таблица (9 секторов)
0x3200─0x7BFF  ── свободно ──
0x7C00─0x7DFF  Boot сектор (Stage 1, 512B)
0x7E00─0x99FF  Корневой каталог FAT12 (14 секторов)
0x8000─0x8FFF  Stage 2 код (внутри root dir, допустимо)
0x10000─...    KERNEL.BIN (до 64 КБ)
```

## BootInfo структура (0x0500)

```c
struct BootInfo {
    uint8_t  boot_drive;         // 0x0500 — номер диска BIOS
    uint8_t  video_mode;         // 0x0501 — видеорежим (0x03 = 80×25 text)
    uint16_t mem_below_16m_kb;   // 0x0502 — RAM до 16 МБ (КБ)
    uint16_t mem_above_16m_64kb; // 0x0504 — RAM выше 16 МБ (блоки 64 КБ)
    uint32_t magic;              // 0x0506 — 0xB0071AF0
};
```

Ядро проверяет `magic == 0xB0071AF0` и читает данные. Команда `bootinfo` в shell.

## BPB (BIOS Parameter Block)

Stage 1 содержит BPB для FAT12 совместимости. При сборке `mkfs.fat` перезаписывает поля BPB в образе, наш asm-шаблон — fallback. Stage 2 читает BPB напрямую из памяти `0x7C00+offset`.

| Поле | Оффсет | Значение |
|------|--------|----------|
| SectorsPerCluster | 0x0D | 1 |
| ReservedSectors | 0x0E | 1 |
| NumberOfFATs | 0x10 | 2 |
| RootEntries | 0x11 | 224 |
| TotalSectors | 0x13 | 2880 |
| SectorsPerFAT | 0x16 | 9 |
| SectorsPerTrack | 0x18 | 18 |
| HeadsPerCylinder | 0x1A | 2 |

## Сборка и запуск

```bash
bash build.sh
qemu-system-i386 -fda disk.img -hda data.img -boot a
```

Скрипт `build.sh`:
1. Собирает `bootloader.bin` и `STAGE2.BIN` через NASM
2. Компилирует и линкует ядро → `KERNEL.BIN`
3. Создаёт `disk.img` (1.44 МБ FAT12), записывает boot-сектор
4. Копирует `STAGE2.BIN` первым (получает кластер 2, гарантированно непрерывный)
5. Копирует `KERNEL.BIN` (загружается через FAT12 цепочку)

## Ограничения

- Максимальный размер ядра: ~1.3 МБ (ограничение FAT12 на 1.44 МБ дискете)
- Stage 2 предполагает контигуозность STAGE2.BIN (первые 8 секторов)
- Только CHS-адресация (совместимость с флоппи)
- Real Mode до передачи управления ядру (Protected Mode включается в `kernel_entry.asm`)
