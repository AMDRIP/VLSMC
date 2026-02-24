/**
 * @file types.h
 * @brief Базовые типы, перечисления и структуры ядра RAND Elecorner 36.
 *
 * Этот файл содержит фундаментальные определения, используемые
 * всеми подсистемами ядра. Не зависит от Qt и других библиотек GUI.
 */

#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>
#include <variant>
#include <optional>
#include <chrono>

namespace re36 {

// ============================================================================
// Базовые типовые алиасы
// ============================================================================

using Pid          = uint32_t;     ///< Идентификатор процесса
using Uid          = uint32_t;     ///< Идентификатор пользователя
using Gid          = uint32_t;     ///< Идентификатор группы
using InodeId      = uint32_t;     ///< Идентификатор инода
using BlockId      = uint32_t;     ///< Идентификатор блока на диске
using DeviceId     = uint16_t;     ///< Идентификатор устройства
using SemaphoreId  = uint32_t;     ///< Идентификатор семафора
using MutexId      = uint32_t;     ///< Идентификатор мьютекса
using PipeId       = uint32_t;     ///< Идентификатор канала
using MessageQueueId = uint32_t;   ///< Идентификатор очереди сообщений
using Tick         = uint64_t;     ///< Номер тика (квант времени)
using VirtualAddr  = uint32_t;     ///< Виртуальный адрес
using PhysicalAddr = uint32_t;     ///< Физический адрес
using PageNumber   = uint32_t;     ///< Номер страницы
using FrameNumber  = uint32_t;     ///< Номер фрейма

/// Константа: отсутствующий PID
constexpr Pid INVALID_PID = 0;

/// Константа: PID ядра
constexpr Pid KERNEL_PID = 1;

/// Константа: UID суперпользователя
constexpr Uid ROOT_UID = 0;

// ============================================================================
// Состояния ядра
// ============================================================================

/**
 * @enum KernelState
 * @brief Состояние ядра ОС.
 */
enum class KernelState : uint8_t {
    Uninitialized,   ///< Ядро не инициализировано
    Booting,         ///< Идёт загрузка и инициализация подсистем
    Running,         ///< Ядро работает в штатном режиме
    ShuttingDown,    ///< Идёт остановка подсистем
    Halted           ///< Ядро остановлено
};

// ============================================================================
// Состояния процесса
// ============================================================================

/**
 * @enum ProcessState
 * @brief Состояние процесса в жизненном цикле.
 */
enum class ProcessState : uint8_t {
    New,             ///< Процесс создан, ещё не готов
    Ready,           ///< В очереди готовых, ожидает CPU
    Running,         ///< Выполняется на CPU
    Waiting,         ///< Ожидает событие (I/O, IPC и т.д.)
    Terminated       ///< Завершён
};

// ============================================================================
// Алгоритмы планировщика
// ============================================================================

/**
 * @enum SchedulerAlgorithm
 * @brief Доступные алгоритмы планирования CPU.
 */
enum class SchedulerAlgorithm : uint8_t {
    FCFS,            ///< First Come, First Served
    RoundRobin,      ///< Циклическое планирование с квантом
    Priority,        ///< Приоритетное (preemptive)
    MultilevelQueue  ///< Многоуровневая очередь
};

// ============================================================================
// Алгоритмы управления памятью
// ============================================================================

/**
 * @enum AllocationAlgorithm
 * @brief Алгоритм выделения памяти.
 */
enum class AllocationAlgorithm : uint8_t {
    FirstFit,        ///< Первый подходящий
    BestFit,         ///< Наилучший подходящий
    WorstFit         ///< Наихудший подходящий
};

/**
 * @enum PageReplacementAlgorithm
 * @brief Алгоритм замещения страниц.
 */
enum class PageReplacementAlgorithm : uint8_t {
    FIFO,            ///< First In, First Out
    LRU,             ///< Least Recently Used
    OPT              ///< Оптимальный (предсказание будущего)
};

// ============================================================================
// Типы устройств
// ============================================================================

/**
 * @enum DeviceType
 * @brief Тип виртуального устройства.
 */
enum class DeviceType : uint8_t {
    Block,           ///< Блочное устройство (диск)
    Character        ///< Символьное устройство (клавиатура, принтер)
};

/**
 * @enum DeviceStatus
 * @brief Текущий статус устройства.
 */
enum class DeviceStatus : uint8_t {
    Idle,            ///< Устройство свободно
    Busy,            ///< Устройство занято обработкой запроса
    Error,           ///< Ошибка устройства
    Offline          ///< Устройство отключено
};

// ============================================================================
// Алгоритмы планирования I/O
// ============================================================================

/**
 * @enum DiskSchedulingAlgorithm
 * @brief Алгоритм планирования дисковых запросов.
 */
enum class DiskSchedulingAlgorithm : uint8_t {
    FCFS,            ///< First Come, First Served
    SSTF,            ///< Shortest Seek Time First
    SCAN             ///< Алгоритм лифта
};

// ============================================================================
// Права доступа к файлам
// ============================================================================

/**
 * @struct FilePermissions
 * @brief Права доступа rwx для владельца, группы и остальных.
 */
struct FilePermissions {
    bool ownerRead    : 1;
    bool ownerWrite   : 1;
    bool ownerExecute : 1;
    bool groupRead    : 1;
    bool groupWrite   : 1;
    bool groupExecute : 1;
    bool otherRead    : 1;
    bool otherWrite   : 1;
    bool otherExecute : 1;

    /// Создать права по умолчанию (rwxr-xr-x)
    static FilePermissions defaultFile() {
        return {true, true, false, true, false, false, true, false, false};
    }

    /// Создать права для каталога (rwxr-xr-x)
    static FilePermissions defaultDirectory() {
        return {true, true, true, true, false, true, true, false, true};
    }

    /// Преобразовать в строку формата "rwxr-xr-x"
    std::string toString() const;
};

// ============================================================================
// Уровень серьёзности событий
// ============================================================================

/**
 * @enum LogLevel
 * @brief Уровень журнала событий ядра.
 */
enum class LogLevel : uint8_t {
    Debug,
    Info,
    Warning,
    Error,
    Critical
};

// ============================================================================
// Типы событий шины
// ============================================================================

/**
 * @enum EventType
 * @brief Категории событий, передаваемых через EventBus.
 */
enum class EventType : uint16_t {
    // Процессы
    ProcessCreated,
    ProcessTerminated,
    ProcessStateChanged,
    ContextSwitch,

    // Память
    MemoryAllocated,
    MemoryFreed,
    PageFault,
    PageReplaced,
    PageSwappedOut,      ///< Страница выгружена в swap
    PageSwappedIn,       ///< Страница загружена из swap

    // Файловая система
    FileCreated,
    FileDeleted,
    FileModified,
    DirectoryCreated,
    DirectoryDeleted,

    // I/O
    IoRequestQueued,
    IoRequestCompleted,
    InterruptReceived,

    // IPC
    MessageSent,
    MessageReceived,
    SemaphoreAcquired,
    SemaphoreReleased,
    DeadlockDetected,

    // Пользователи
    UserLoggedIn,
    UserLoggedOut,
    PermissionDenied,

    // Приложения
    AppInstalled,
    AppUninstalled,
    AppStarted,
    AppStopped,

    // Системные
    KernelBooted,
    KernelShutdown,
    SyscallInvoked,
    TickCompleted
};

// ============================================================================
// Конфигурация ядра
// ============================================================================

/**
 * @struct KernelConfig
 * @brief Параметры, задаваемые при загрузке ядра.
 *
 * Пользователь может настраивать их через экран загрузки (BOOT-03).
 */
struct KernelConfig {
    // --- Память ---
    size_t totalPhysicalMemory  = 1024 * 1024;  ///< Всего физической памяти (байт), по умолчанию 1 МБ
    size_t pageSize             = 4096;          ///< Размер одной страницы (байт)
    AllocationAlgorithm allocationAlgorithm     = AllocationAlgorithm::FirstFit;
    PageReplacementAlgorithm pageReplacement    = PageReplacementAlgorithm::LRU;
    size_t swapSize             = 0;              ///< Размер swap (0 = 25% RAM по умолчанию)
    bool   swapEnabled          = true;           ///< Подкачка включена?

    // --- Планировщик ---
    SchedulerAlgorithm schedulerAlgorithm       = SchedulerAlgorithm::RoundRobin;
    uint32_t timeQuantum        = 4;             ///< Квант времени для Round Robin (тики)
    uint32_t maxProcesses       = 256;           ///< Максимум процессов одновременно

    // --- Файловая система ---
    size_t diskSize             = 16 * 1024 * 1024; ///< Размер виртуального диска (байт), по умолчанию 16 МБ
    size_t blockSize            = 512;           ///< Размер блока на диске (байт)
    uint32_t maxInodes          = 4096;          ///< Максимум инодов

    // --- I/O ---
    DiskSchedulingAlgorithm diskScheduling      = DiskSchedulingAlgorithm::SCAN;

    // --- Тики ---
    uint32_t tickIntervalMs     = 16;            ///< Интервал между тиками (мс), ~60 fps

    // --- Пользователи ---
    std::string rootPassword    = "root";        ///< Пароль суперпользователя по умолчанию
    std::string hostname        = "rand-elecorner-36"; ///< Имя хоста
};

// ============================================================================
// Универсальный тип для аргументов/результатов
// ============================================================================

/**
 * @typedef KernelValue
 * @brief Вариантный тип для передачи данных в/из системных вызовов.
 */
using KernelValue = std::variant<
    std::monostate,     // пусто
    bool,
    int64_t,
    uint64_t,
    double,
    std::string,
    std::vector<uint8_t>
>;

} // namespace re36
