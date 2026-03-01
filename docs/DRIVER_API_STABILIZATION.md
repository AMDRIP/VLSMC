# DriverAPI — план стабилизации

> Цель: перевести текущий DriverAPI из режима «доверенной демо-модели» в режим предсказуемого и безопасного ABI для долгоживущих Ring3-драйверов.

## 1) Текущее состояние (по коду)

1. User-space `DriverAPI` задаёт только C++-интерфейс `init/read/write/seek/ioctl/stop` без ABI-версии, feature-flags и без стандартной модели ошибок.
2. Syscall-слой даёт прямой доступ к `inb/outb/inw/outw`, `wait_irq`, `map_mmio`, `grant_mmio`, `set_driver`.
3. Сейчас есть «временный» bypass: все ELF, загруженные через `elf_exec`, получают `is_driver = true`.
4. В `sys_map_mmio` есть логическая ошибка проверки grant-диапазонов: для `is_driver == true` доступ разрешается без проверки списка grant.
5. В DriverContext отсутствует жизненный цикл capability-хэндлов (IRQ/MMIO/PIO), поэтому драйвер не может безопасно «освобождать» возможности.
6. `PS2Driver` работает через busy-wait и не имеет явной state-машины и recovery-политики.

## 2) Главные риски стабильности

- **Эскалация привилегий**: auto-driver для любого ELF и широкие I/O syscalls.
- **Непредсказуемость ABI**: нет version negotiation для user driver binary.
- **Дрейф поведения ошибок**: разные syscalls возвращают `0`/`-1` без единой errno-модели.
- **Зависание в I/O**: нет timeout/error surface в ожидании IRQ и I/O handshakes.
- **Невозможность безопасного hot-restart**: нет единого stop/teardown протокола в ядре.

## 3) Реализации для стабилизации (рекомендуемый порядок)

## 3.1 Capability-first DriverAPI (MVP)

Вместо «глобального is_driver» ввести capability-токены, выдаваемые ядром:

- `cap_irq_wait(irq)`
- `cap_pio_read(port, width)` / `cap_pio_write(...)`
- `cap_mmio_map(cap_id, virt, pages)`

Идея: `sys_set_driver` переводится в «может запрашивать capabilities», а не «полный доступ к железу».

### Минимальные syscall-добавления

- `SYS_CAP_ACQUIRE(type, arg0, arg1)` -> `cap_id | <0`
- `SYS_CAP_RELEASE(cap_id)`
- `SYS_CAP_PIO_RW(cap_id, op, value)`
- `SYS_CAP_IRQ_WAIT(cap_id, timeout_ms)`
- `SYS_CAP_MMIO_MAP(cap_id, virt, pages)`

Это сразу решает гранулярность прав и даёт обратимый teardown.

## 3.2 Исправление политики MMIO (обязательный hotfix)

Текущее поведение: `if (cur.is_driver) allowed = true;`.

Исправление:

- Разрешать `map_mmio` **только** если диапазон входит в `allowed_mmio[]`.
- Исключение только для `tid==0` (kernel bootstrap thread), и то под compile-time флагом `RE36_DEV_TRUST_BOOT=1`.

## 3.3 Driver manifest + trust policy

Для ELF-драйвера рядом класть `*.drv.json`:

```json
{
  "name": "ps2kbd",
  "abi": 1,
  "caps": {
    "irq": [1],
    "pio": [{"start":96,"end":100}],
    "mmio": []
  },
  "restart": "on-failure"
}
```

Kernel loader:

1. читает manifest,
2. валидирует ABI,
3. выдаёт caps,
4. только после этого запускает driver thread.

## 3.4 DriverContext v2 (ABI-stable)

Предлагаемая структура:

```c
struct DriverContextV2 {
    uint32_t abi_version;      // = 2
    uint32_t driver_tid;
    uint32_t device_id;
    uint32_t flags;

    uint32_t irq_caps[8];
    uint32_t pio_caps[8];
    uint32_t mmio_caps[8];

    void*    shared_cfg;
    uint32_t shared_cfg_size;

    int32_t  last_error;       // errno-like
};
```

Плюс `ioctl`-контракт:

- `DRV_IOCTL_GET_VERSION`
- `DRV_IOCTL_GET_HEALTH`
- `DRV_IOCTL_RESET`
- `DRV_IOCTL_QUIESCE`

## 3.5 Единая ошибка/диагностика

Ввести таблицу для драйверного слоя:

- `DRV_OK = 0`
- `DRV_EPERM`, `DRV_EINVAL`, `DRV_ETIMEDOUT`, `DRV_EIO`, `DRV_EBUSY`, `DRV_EAGAIN`, `DRV_ENOTSUP`.

Правило: syscall всегда возвращает `<0` при ошибке и заполняет `last_error`.

## 3.6 Надёжный lifecycle драйвера

Ядро должно поддерживать единый протокол:

1. `probe` (resource check)
2. `init`
3. `run`
4. `quiesce`
5. `stop`
6. `recover` (опционально)

Технически: watchdog-счётчик heartbeat + restart policy (`never`, `on-failure`, `always`).

## 3.7 Стабилизация PS/2 как эталонного драйвера

Для `PS2Driver` внедрить:

- state-machine (`Reset -> SelfTest -> Config -> Streaming`),
- timeout-aware `wait_read/write` с кодами ошибок,
- retry budget (например, 3 попытки для ACK),
- отдельный ring buffer событий,
- `ioctl(GET_STATS)` для диагностики (timeouts, parity, overruns).

## 4) План внедрения (по PR)

### PR-1 (без ломки API)

- Hotfix `sys_map_mmio` policy.
- Убрать auto `is_driver=true` для всех ELF, оставить whitelist (shell/testdriver).
- Унифицировать return codes (`-1`/negative errno).

### PR-2 (новые syscall capabilities)

- Добавить `SYS_CAP_*`.
- Обернуть существующие `sys_inb/outb/...` через capability checks.
- Добавить revoke path в `thread_cleanup`.

### PR-3 (manifest + loader)

- Парсинг `.drv.json`.
- Выдача caps из manifest.
- Запуск/перезапуск по policy.

### PR-4 (DriverContext v2 + PS2 refactor)

- Ввести `DriverContextV2`.
- Перенести PS2 driver на v2 + telemetry.

## 5) Критерии готовности (Definition of Done)

- Драйвер без capability не может:
  - делать PIO,
  - ждать IRQ,
  - map/unmap MMIO.
- После `thread_cleanup` все caps ревокнуты и MMIO unmapped.
- Driver ABI version check обязателен при старте.
- Watchdog фиксирует зависшие драйверы и применяет policy без kernel panic.
- Нагрузочный тест (10k циклов init/stop) без утечек и зависаний.

## 6) Быстрые точечные изменения прямо сейчас

1. В `sys_map_mmio` убрать unconditional `allowed=true` для `is_driver`.
2. В `elf_exec` убрать выдачу driver-флага всем ELF.
3. В `PS2Driver::wait_read/write` возвращать код таймаута и прокидывать его в `read/write/init`.
4. Ввести единый `driver_errno.h` для user-space драйверов.

Эти четыре пункта дадут максимум прироста стабильности при минимальном размере изменений.
