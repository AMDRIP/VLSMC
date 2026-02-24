# RAND Elecorner 36 — Стек Технологий

> Build 0001

---

## Обзор

RAND Elecorner 36 — десктопное приложение-симулятор ОС, построенное на трёх основных языках:

```
  C++ (ядро)  ←→  Qt 6 / QML (GUI)  ←→  Python (тесты/скрипты)
                       ↑
                   QJSEngine
                       ↑
              TypeScript → JS (приложения)
```

---

## Основные технологии

### C++17 — Ядро ОС

| Параметр | Значение |
|----------|---------|
| **Стандарт** | C++17 (минимум) |
| **Компилятор** | MSVC 2022 (Windows), GCC 13+ (Linux), Clang 16+ (macOS) |
| **Роль** | Вся логика ядра: планировщик, менеджер памяти, ФС, IPC, I/O |
| **Тестирование** | GoogleTest + GoogleMock |

**Ключевые библиотеки C++:**

| Библиотека | Назначение | Лицензия |
|-----------|-----------|----------|
| Qt 6.6+ | GUI-фреймворк | LGPL v3 / Commercial |
| nlohmann/json | Парсинг JSON (manifest, конфиги) | MIT |
| spdlog | Логирование | MIT |
| GoogleTest | Unit-тесты C++ | BSD-3 |

---

### Qt 6 + QML — Графический интерфейс

| Параметр | Значение |
|----------|---------|
| **Версия** | Qt 6.6+ |
| **Модули** | QtCore, QtGui, QtWidgets, QtQuick (QML), QtQml |
| **JS-движок** | QJSEngine (встроен в QtQml) |
| **Стилизация** | QML + пользовательские стили, темы через Qt Quick Controls |

**Используемые возможности Qt:**

| Модуль | Для чего |
|--------|---------|
| **QtWidgets** | Основные диалоги и системные окна |
| **QML / Qt Quick** | Анимации загрузки, рабочий стол, оконный менеджер |
| **QJSEngine** | Выполнение пользовательских JS-приложений |
| **QFileSystemModel** | Визуализация файлового дерева |
| **Qt Charts** | Графики загрузки CPU, памяти |

---

### Python 3.12 + PySide6 — Скрипты и Тесты

| Параметр | Значение |
|----------|---------|
| **Версия** | Python 3.12+ |
| **Qt-биндинги** | PySide6 (официальные Qt for Python) |
| **Тестирование** | pytest + pytest-qt |
| **Менеджер пакетов** | pip / uv |

**Роли Python в проекте:**

| Роль | Описание |
|------|----------|
| **Интеграционные тесты** | Тестирование полных сценариев через PySide6 + pytest |
| **Скрипты демонстрации** | Автоматические сценарии: «запусти 10 процессов и покажи планировщик» |
| **Утилиты сборки** | Генерация ресурсов, упаковка `.vlsmc-pkg` |
| **Прототипирование** | Быстрая проверка алгоритмов перед C++ реализацией |

---

### TypeScript → JavaScript — Приложения ОС

| Параметр | Значение |
|----------|---------|
| **Версия TS** | TypeScript 5.x |
| **Целевой JS** | ES2020 (совместимость с QJSEngine) |
| **Компиляция** | AOT — разработчик компилирует `tsc` перед упаковкой |
| **SDK** | VLSMC App SDK (`@vlsmc/sdk`) |
| **Движок** | QJSEngine (встроен в Qt 6) |

---

## Система сборки

### CMake

| Параметр | Значение |
|----------|---------|
| **Версия** | CMake 3.25+ |
| **Генератор** | Ninja (рекомендуется) или Visual Studio |
| **Пресеты** | `debug`, `release`, `test` |

**Структура CMake:**

```
CMakeLists.txt              # Корневой — подключает модули
├── kernel/CMakeLists.txt   # Библиотека ядра (static lib)
├── app/CMakeLists.txt      # Qt-приложение (executable)
└── tests/CMakeLists.txt    # GoogleTest тесты
```

### Типичные команды

```bash
# Конфигурация
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug

# Сборка
cmake --build build

# Запуск
./build/app/rand-elecorner-36

# Тесты C++
cd build && ctest --output-on-failure

# Тесты Python
pytest tests/python/ -v
```

---

## Управление зависимостями

| Инструмент | Для чего |
|-----------|---------|
| **vcpkg** или **conan** | C++ зависимости (nlohmann/json, spdlog, gtest) |
| **pip / uv** | Python зависимости (PySide6, pytest) |
| **npm** | TypeScript SDK для разработчиков приложений |

---

## Минимальные системные требования

| Параметр | Значение |
|----------|---------|
| **ОС** | Windows 10+, Ubuntu 22.04+, macOS 13+ |
| **RAM** | 4 ГБ |
| **Дисковое пространство** | 500 МБ |
| **GPU** | Поддержка OpenGL 3.3+ (для Qt Quick) |
| **Разрешение** | 1280 × 720 минимум |

---

## Структура проекта

```
VLSMC/
├── CMakeLists.txt
├── README.md
│
├── kernel/                   # C++ — ядро ОС
│   ├── CMakeLists.txt
│   ├── include/kernel/
│   │   ├── kernel.h
│   │   ├── process.h
│   │   ├── scheduler.h
│   │   ├── memory_manager.h
│   │   ├── filesystem.h
│   │   ├── ipc.h
│   │   ├── io_manager.h
│   │   ├── event_bus.h
│   │   └── syscalls.h
│   └── src/
│       ├── kernel.cpp
│       ├── scheduler.cpp
│       ├── memory_manager.cpp
│       ├── filesystem.cpp
│       ├── ipc.cpp
│       ├── io_manager.cpp
│       ├── event_bus.cpp
│       └── syscalls.cpp
│
├── app/                      # Qt 6 — GUI приложение
│   ├── CMakeLists.txt
│   ├── main.cpp
│   ├── src/
│   │   ├── desktop/          # Рабочий стол, панель задач
│   │   ├── windows/          # Оконный менеджер
│   │   ├── apps/             # Встроенные приложения
│   │   │   ├── terminal/
│   │   │   ├── task_manager/
│   │   │   ├── file_manager/
│   │   │   └── system_monitor/
│   │   ├── boot/             # Экран загрузки
│   │   ├── login/            # Экран входа
│   │   └── app_runtime/      # JS-движок, песочница, UI Bridge
│   ├── qml/                  # QML-файлы интерфейса
│   └── resources/            # Иконки, шрифты, темы
│
├── sdk/                      # TypeScript App SDK
│   ├── package.json
│   ├── tsconfig.json
│   └── src/
│       ├── index.ts
│       └── types.ts
│
├── backend/                  # Python — скрипты и тесты
│   ├── pyproject.toml
│   ├── scripts/              # Демонстрационные сценарии
│   ├── tools/                # Утилиты (упаковщик .vlsmc-pkg и др.)
│   └── tests/
│       ├── test_scheduler.py
│       ├── test_memory.py
│       └── test_filesystem.py
│
├── tests/                    # C++ тесты (GoogleTest)
│   ├── CMakeLists.txt
│   ├── test_scheduler.cpp
│   ├── test_memory.cpp
│   └── test_filesystem.cpp
│
└── docs/                     # Документация
    ├── PROJECT_PLAN.md
    ├── REQUIREMENTS.md
    ├── TECH_STACK.md
    ├── ARCHITECTURE.md
    ├── APP_SDK.md
    └── PACKAGE_FORMAT.md
```
