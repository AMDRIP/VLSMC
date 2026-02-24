/**
 * @file memory_manager.h
 * @brief Менеджер памяти RAND Elecorner 36.
 *
 * Управляет физической и виртуальной памятью, реализует
 * страничную организацию, алгоритмы выделения и замещения.
 * Соответствует требованиям MEM-01 — MEM-07.
 */

#pragma once

#include "types.h"

#include <vector>
#include <unordered_map>
#include <optional>
#include <deque>

namespace re36 {

// Предварительные объявления
class EventBus;

// ============================================================================
// Фрейм физической памяти
// ============================================================================

/**
 * @struct MemoryFrame
 * @brief Один фрейм (кадр) физической памяти.
 */
struct MemoryFrame {
    FrameNumber     frameId     = 0;
    bool            isFree      = true;         ///< Фрейм свободен?
    bool            isKernel    = false;         ///< Зарезервирован ядром?
    Pid             ownerPid    = INVALID_PID;   ///< PID владеющего процесса
    PageNumber      pageNumber  = 0;             ///< Виртуальная страница
    Tick            lastAccess  = 0;             ///< Тик последнего обращения (для LRU)
    Tick            loadedAt    = 0;             ///< Тик загрузки (для FIFO)
    bool            dirty       = false;         ///< Страница была модифицирована?
    bool            referenced  = false;         ///< Было обращение с последней проверки?
};

// ============================================================================
// Запись таблицы страниц
// ============================================================================

/**
 * @struct PageTableEntry
 * @brief Одна запись в таблице страниц процесса.
 */
struct PageTableEntry {
    PageNumber      pageNumber  = 0;
    FrameNumber     frameNumber = 0;
    bool            present     = false;        ///< Страница в ОЗУ?
    bool            dirty       = false;        ///< Была модифицирована?
    bool            referenced  = false;        ///< Было обращение?
    bool            inSwap      = false;        ///< Страница выгружена в swap?
    uint32_t        swapSlot    = UINT32_MAX;   ///< Номер слота в swap (UINT32_MAX = нет)
    Tick            lastAccess  = 0;
};

// ============================================================================
// Swap-пространство
// ============================================================================

/**
 * @struct SwapSlot
 * @brief Один слот подкачки на виртуальном диске.
 */
struct SwapSlot {
    uint32_t    slotId      = 0;
    bool        isFree      = true;         ///< Слот свободен?
    Pid         ownerPid    = INVALID_PID;  ///< PID владельца
    PageNumber  pageNumber  = 0;            ///< Виртуальная страница
    bool        dirty       = false;        ///< Данные были модифицированы до выгрузки?
    Tick        swappedAt   = 0;            ///< Тик выгрузки
};

/**
 * @struct SwapStats
 * @brief Статистика подсистемы подкачки.
 */
struct SwapStats {
    size_t      totalSlots      = 0;    ///< Всего слотов swap
    size_t      usedSlots       = 0;    ///< Занятых слотов
    size_t      freeSlots       = 0;    ///< Свободных слотов
    double      usagePercent    = 0.0;  ///< % использования swap
    uint64_t    totalSwapOuts   = 0;    ///< Всего выгрузок (RAM → Swap)
    uint64_t    totalSwapIns    = 0;    ///< Всего загрузок (Swap → RAM)
    uint64_t    dirtySwapOuts   = 0;    ///< Выгрузок dirty-страниц (с записью)
    size_t      totalSwapBytes  = 0;    ///< Общий объём swap в байтах
    size_t      usedSwapBytes   = 0;    ///< Использовано swap в байтах
};

/**
 * @enum SwapEvent
 * @brief Тип события подкачки (для визуализации).
 */
enum class SwapEventType : uint8_t {
    SwapOut,    ///< Выгрузка из RAM в Swap
    SwapIn      ///< Загрузка из Swap в RAM
};

/**
 * @struct SwapLogEntry
 * @brief Запись в журнале операций подкачки.
 */
struct SwapLogEntry {
    Tick            tick;
    SwapEventType   type;
    Pid             pid;
    PageNumber      page;
    uint32_t        swapSlot;
    FrameNumber     frame;          ///< Фрейм RAM (откуда/куда)
    bool            wasDirty;       ///< Страница была dirty при swap out?
};

// ============================================================================
// Сегмент памяти (для визуализации карты)
// ============================================================================

/**
 * @struct MemorySegment
 * @brief Непрерывный участок памяти (для карты).
 */
struct MemorySegment {
    PhysicalAddr    startAddr;
    size_t          size;
    bool            isFree;
    bool            isKernel;
    Pid             ownerPid;
    std::string     label;      ///< Описание ("Kernel", "Process 5", "Free")
};

// ============================================================================
// Статистика памяти
// ============================================================================

/**
 * @struct MemoryStats
 * @brief Статистика подсистемы памяти.
 */
struct MemoryStats {
    size_t      totalMemory         = 0;    ///< Всего физической памяти
    size_t      usedMemory          = 0;    ///< Занято
    size_t      freeMemory          = 0;    ///< Свободно
    size_t      kernelMemory        = 0;    ///< Занято ядром
    uint32_t    totalFrames         = 0;    ///< Всего фреймов
    uint32_t    usedFrames          = 0;    ///< Занятых фреймов
    uint32_t    freeFrames          = 0;    ///< Свободных фреймов
    uint64_t    pageFaults          = 0;    ///< Всего page faults
    uint64_t    pageHits            = 0;    ///< Всего page hits
    uint64_t    pageReplacements    = 0;    ///< Всего замещений страниц
    double      hitRate             = 0.0;  ///< Процент попаданий
    double      usagePercent        = 0.0;  ///< Процент использования
    double      fragmentation       = 0.0;  ///< Внешняя фрагментация (MEM-06)
    SwapStats   swap;                       ///< Статистика подкачки
};

/**
 * @struct MemoryManagerSnapshot
 * @brief Снимок для SystemMonitor.
 */
struct MemoryManagerSnapshot {
    uint32_t    totalFrames         = 0;
    uint32_t    usedFrames          = 0;
    uint64_t    totalPageFaults     = 0;
    double      fragmentationPercent = 0.0;
    SwapStats   swap;
};

// ============================================================================
// Менеджер памяти
// ============================================================================

/**
 * @class MemoryManager
 * @brief Управление физической и виртуальной памятью.
 */
class MemoryManager {
public:
    explicit MemoryManager(EventBus& eventBus, const KernelConfig& config);
    ~MemoryManager();

    MemoryManager(const MemoryManager&) = delete;
    MemoryManager& operator=(const MemoryManager&) = delete;

    // ========================================================================
    // Инициализация
    // ========================================================================

    /**
     * Инициализировать подсистему памяти.
     * Создаёт массив фреймов и резервирует память для ядра.
     */
    bool init();

    // ========================================================================
    // Выделение / освобождение (MEM-05, MEM-07)
    // ========================================================================

    /**
     * Выделить память для процесса.
     * @param pid    PID процесса
     * @param size   Запрашиваемый объём (байт)
     * @return Базовый виртуальный адрес или nullopt при нехватке
     */
    std::optional<VirtualAddr> allocate(Pid pid, size_t size);

    /**
     * Освободить всю память процесса.
     * @param pid PID процесса
     */
    void deallocate(Pid pid);

    /**
     * Освободить конкретный диапазон.
     * @param pid  PID процесса
     * @param addr Виртуальный адрес начала
     * @param size Размер
     */
    void deallocate(Pid pid, VirtualAddr addr, size_t size);

    // ========================================================================
    // Страничная организация (MEM-02, MEM-03, MEM-04)
    // ========================================================================

    /**
     * Обратиться к виртуальному адресу (может вызвать page fault).
     * @param pid   PID процесса
     * @param vAddr Виртуальный адрес
     * @param write Операция записи? (устанавливает dirty bit)
     * @return Физический адрес или nullopt при невозможности обслужить
     */
    std::optional<PhysicalAddr> accessMemory(Pid pid, VirtualAddr vAddr, bool write = false);

    /**
     * Проверить и обработать отложенные page faults.
     * Вызывается из Kernel::tick().
     */
    void checkPageFaults();

    // ========================================================================
    // Настройка алгоритмов
    // ========================================================================

    /// Сменить алгоритм выделения (MEM-05)
    void setAllocationAlgorithm(AllocationAlgorithm algo);
    AllocationAlgorithm getAllocationAlgorithm() const;

    /// Сменить алгоритм замещения страниц (MEM-04)
    void setPageReplacementAlgorithm(PageReplacementAlgorithm algo);
    PageReplacementAlgorithm getPageReplacementAlgorithm() const;

    // ========================================================================
    // Визуализация и запросы
    // ========================================================================

    /// Полная статистика памяти
    MemoryStats getStats() const;

    /// Снимок для SystemMonitor
    MemoryManagerSnapshot getSnapshot() const;

    /// Карта памяти (для визуализации)
    std::vector<MemorySegment> getMemoryMap() const;

    /// Таблица страниц процесса
    std::vector<PageTableEntry> getPageTable(Pid pid) const;

    /// Состояние всех фреймов
    std::vector<MemoryFrame> getFrames() const;

    /// Объём памяти, выделенный процессу
    size_t getProcessMemoryUsage(Pid pid) const;

    // ========================================================================
    // Подкачка / Swap (MEM-08)
    // ========================================================================

    /// Включить/выключить swap
    void setSwapEnabled(bool enabled);
    bool isSwapEnabled() const;

    /// Установить размер swap (в байтах, пересоздаёт массив слотов)
    void setSwapSize(size_t bytes);

    /// Статистика подкачки
    SwapStats getSwapStats() const;

    /// Состояние всех swap-слотов (для визуализации)
    std::vector<SwapSlot> getSwapSlots() const;

    /// Журнал операций подкачки (последние N)
    std::vector<SwapLogEntry> getSwapLog(size_t count) const;

    /// Принудительно выгрузить страницу в swap (для демонстрации)
    bool swapOutPage(Pid pid, PageNumber page);

    /// Принудительно загрузить страницу из swap
    bool swapInPage(Pid pid, PageNumber page);

private:
    EventBus&                                   eventBus_;
    KernelConfig                                config_;

    // Физическая память (массив фреймов, MEM-01)
    std::vector<MemoryFrame>                    frames_;
    uint32_t                                    totalFrames_    = 0;

    // Таблицы страниц процессов
    std::unordered_map<Pid, std::vector<PageTableEntry>> pageTables_;

    // Алгоритмы
    AllocationAlgorithm                          allocAlgo_;
    PageReplacementAlgorithm                     replaceAlgo_;

    // Статистика
    uint64_t                                     pageFaults_     = 0;
    uint64_t                                     pageHits_       = 0;
    uint64_t                                     replacements_   = 0;

    // Размер зарезервированной памяти ядра
    uint32_t                                     kernelFrames_   = 0;

    // Swap-пространство
    bool                                         swapEnabled_    = true;
    std::vector<SwapSlot>                        swapSlots_;
    uint32_t                                     totalSwapSlots_ = 0;
    uint64_t                                     swapOuts_       = 0;
    uint64_t                                     swapIns_        = 0;
    uint64_t                                     dirtySwapOuts_  = 0;
    std::deque<SwapLogEntry>                     swapLog_;
    size_t                                       maxSwapLog_     = 500;

    // --- Внутренние методы ---

    /// Найти свободный фрейм по алгоритму выделения
    std::optional<FrameNumber> findFreeFrame(size_t contiguousCount = 1);

    /// Выбрать страницу-жертву для замещения
    FrameNumber selectVictimFrame();

    /// FIFO замещение
    FrameNumber selectVictimFIFO();

    /// LRU замещение
    FrameNumber selectVictimLRU();

    /// Оптимальное замещение (симуляция будущего)
    FrameNumber selectVictimOPT();

    /// Загрузить страницу в фрейм
    void loadPage(Pid pid, PageNumber page, FrameNumber frame, Tick currentTick);

    /// Выгрузить страницу из фрейма (записать в swap если dirty)
    void evictFrame(FrameNumber frame);

    /// Рассчитать внешнюю фрагментацию
    double calculateFragmentation() const;

    // --- Swap внутренние ---

    /// Найти свободный слот в swap
    std::optional<uint32_t> findFreeSwapSlot();

    /// Записать страницу в swap-слот
    void writeToSwap(Pid pid, PageNumber page, FrameNumber frame, uint32_t slot, Tick tick);

    /// Прочитать страницу из swap-слота
    void readFromSwap(uint32_t slot, Pid pid, PageNumber page, FrameNumber frame, Tick tick);

    /// Освободить все swap-слоты процесса
    void freeSwapSlots(Pid pid);
};

} // namespace re36
