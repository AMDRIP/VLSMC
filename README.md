# RAND Elecorner 36

> Концептуальный симулятор операционной системы | Build 0001

---

## О проекте

**RAND Elecorner 36** — десктопное приложение, моделирующее работу операционной системы с полноценным графическим интерфейсом. Симулятор включает управление процессами, памятью, файловой системой, устройствами, межпроцессное взаимодействие, терминал и платформу пользовательских приложений.

## Стек технологий

| Компонент | Технология |
|-----------|-----------|
| Ядро ОС | C++17 |
| GUI | Qt 6 + QML |
| Скрипты / Тесты | Python 3.12 + PySide6 |
| Приложения | TypeScript → JS (QJSEngine) |
| Сборка | CMake 3.25+ |

## Сборка и запуск

```bash
# Конфигурация
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug

# Сборка
cmake --build build

# Запуск
./build/app/rand-elecorner-36
```

## Документация

| Документ | Описание |
|----------|----------|
| [PROJECT_PLAN.md](docs/PROJECT_PLAN.md) | План и этапы разработки |
| [REQUIREMENTS.md](docs/REQUIREMENTS.md) | Требования к компонентам |
| [TECH_STACK.md](docs/TECH_STACK.md) | Стек технологий |
| [ARCHITECTURE.md](docs/ARCHITECTURE.md) | Системная архитектура |
| [APP_SDK.md](docs/APP_SDK.md) | SDK для разработчиков приложений |
| [PACKAGE_FORMAT.md](docs/PACKAGE_FORMAT.md) | Формат пакетов `.vlsmc-pkg` |

## Лицензия

*TBD*
