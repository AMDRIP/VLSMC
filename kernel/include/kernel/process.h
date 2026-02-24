/**
 * @file process.h
 * @brief Модель процесса (PCB) для RAND Elecorner 36.
 *
 * Process Control Block — основная структура, описывающая процесс
 * в системе. Содержит идентификаторы, состояние, приоритет,
 * информацию о памяти и статистику исполнения.
 */

#pragma once

#include "types.h"

#include <string>
#include <vector>

namespace re36 {

// ============================================================================
// Инструкция процесса
// ============================================================================

/**
 * @enum InstructionType
 * @brief Тип виртуальной инструкции, которую процесс «выполняет».
 */
enum class InstructionType : uint8_t {
    CpuWork,        ///< Чистые вычисления (использует CPU)
    IoRequest,      ///< Запрос к устройству ввода-вывода
    MemoryAccess,   ///< Обращение к памяти (может вызвать page fault)
    IpcOperation,   ///< Межпроцессная коммуникация
    Sleep,          ///< Ожидание (пропуск тиков)
    Exit            ///< Завершение процесса
};

/**
 * @struct Instruction
 * @brief Одна виртуальная инструкция процесса.
 */
struct Instruction {
    InstructionType type    = InstructionType::CpuWork;
    uint32_t        param1  = 0;    ///< Параметр 1 (зависит от типа)
    uint32_t        param2  = 0;    ///< Параметр 2 (зависит от типа)
    std::string     strParam;       ///< Строковый параметр (путь, имя и т.д.)
};

// ============================================================================
// Блок управления процессом (PCB)
// ============================================================================

/**
 * @struct Process
 * @brief Process Control Block — полное описание процесса.
 *
 * Соответствует требованию PROC-01.
 */
struct Process {
    // --- Идентификация ---
    Pid             pid             = INVALID_PID;  ///< Уникальный ID процесса
    Pid             parentPid       = INVALID_PID;  ///< PID родителя (PROC-07)
    std::string     name;                           ///< Имя процесса
    Uid             owner           = ROOT_UID;     ///< Владелец процесса

    // --- Состояние (PROC-02) ---
    ProcessState    state           = ProcessState::New;
    int32_t         exitCode        = 0;            ///< Код завершения

    // --- Приоритет ---
    uint8_t         priority        = 5;            ///< 0 (наивысший) — 255 (низший)
    uint8_t         basePriority    = 5;            ///< Базовый приоритет (не меняется aging'ом)

    // --- Программа (набор инструкций, PROC-06) ---
    std::vector<Instruction> instructions;          ///< Программа процесса
    uint32_t        programCounter  = 0;            ///< Текущая инструкция

    // --- Время ---
    uint32_t        burstTime       = 0;            ///< Общее время CPU (тики)
    uint32_t        remainingBurst  = 0;            ///< Оставшееся время CPU
    uint32_t        waitingTime     = 0;            ///< Время в очереди Ready
    uint32_t        turnaroundTime  = 0;            ///< Полное время от создания до завершения
    Tick            createdAt       = 0;            ///< Тик создания
    Tick            terminatedAt    = 0;            ///< Тик завершения
    uint32_t        ioWaitTime      = 0;            ///< Время ожидания I/O

    // --- Планировщик ---
    uint32_t        quantumRemaining = 0;           ///< Остаток кванта (Round Robin)
    uint8_t         queueLevel      = 0;            ///< Уровень очереди (Multilevel Queue)

    // --- Память ---
    size_t          memoryRequired  = 0;            ///< Запрошенный объём памяти (байт)
    size_t          memoryAllocated = 0;            ///< Реально выделено (байт)
    VirtualAddr     baseAddress     = 0;            ///< Базовый виртуальный адрес
    std::vector<PageNumber> pageTable;              ///< Таблица страниц процесса

    // --- Файловая система ---
    std::string     workingDirectory = "/";         ///< Текущий рабочий каталог

    // --- Дерево процессов (PROC-07) ---
    std::vector<Pid> children;                      ///< Дочерние процессы

    // --- Флаги ---
    bool            isSystemProcess = false;        ///< Системный процесс (нельзя убить обычному юзеру)
    bool            isAppProcess    = false;        ///< Процесс запущен из JS-приложения
    std::string     appId;                          ///< ID приложения (если isAppProcess)

    // ========================================================================
    // Методы
    // ========================================================================

    /// Процесс завершён?
    bool isTerminated() const { return state == ProcessState::Terminated; }

    /// Процесс использует CPU?
    bool isRunning() const { return state == ProcessState::Running; }

    /// Текущая инструкция (nullptr если программа завершена)
    const Instruction* currentInstruction() const;

    /// Перейти к следующей инструкции
    void advanceProgramCounter();

    /// Есть ли ещё инструкции для выполнения?
    bool hasMoreInstructions() const;
};

// ============================================================================
// Вспомогательные структуры
// ============================================================================

/**
 * @struct ProcessSnapshot
 * @brief Краткая информация о процессе для GUI (диспетчер задач).
 */
struct ProcessSnapshot {
    Pid             pid;
    std::string     name;
    ProcessState    state;
    uint8_t         priority;
    size_t          memoryUsed;
    uint32_t        cpuTime;        ///< burstTime - remainingBurst
    std::string     owner;
    bool            isSystem;
};

/**
 * @struct GanttEntry
 * @brief Одна запись диаграммы Ганта для визуализации планировщика.
 */
struct GanttEntry {
    Pid     pid;
    Tick    startTick;
    Tick    endTick;
};

} // namespace re36
