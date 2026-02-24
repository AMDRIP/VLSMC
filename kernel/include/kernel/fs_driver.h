/**
 * @file fs_driver.h
 * @brief Фундаментальный драйвер файловой системы RAND Elecorner 36.
 *
 * FsDriver — низкоуровневый слой между VFS (FileSystem) и виртуальным
 * блочным устройством. Управляет суперблоком, битовыми картами блоков
 * и инодов, блочным кешем (LRU) и простым журналом (WAL).
 *
 * Архитектура виртуального диска:
 *
 *   +──────────────+────────────────+──────────────+───────────+──────────+
 *   │  Суперблок   │ Bitmap блоков  │ Bitmap инодов│ Иноды     │ Данные   │
 *   │  (блок 0)    │ (блок 1..N)    │ (блок N+1..) │ (сырые)   │ (сырые)  │
 *   +──────────────+────────────────+──────────────+───────────+──────────+
 *
 * Ядро общается с драйвером напрямую:
 *   Kernel → FileSystem → FsDriver → виртуальный диск (in-memory)
 *
 * Соответствует требованиям FS-06 (блочное размещение), FS-08 (персистентность).
 */

#pragma once

#include "types.h"

#include <string>
#include <vector>
#include <deque>
#include <unordered_map>
#include <optional>
#include <cstdint>
#include <functional>

namespace re36 {

// Предварительные объявления
class EventBus;

// ============================================================================
// Константы драйвера
// ============================================================================

static constexpr uint32_t FS_MAGIC          = 0x52453336;   ///< "RE36"
static constexpr uint32_t FS_VERSION        = 1;
static constexpr uint32_t DEFAULT_BLOCK_SIZE = 512;         ///< Байт на блок
static constexpr uint32_t DEFAULT_TOTAL_BLOCKS = 4096;      ///< 2 МБ при 512 байт
static constexpr uint32_t DEFAULT_MAX_INODES   = 512;
static constexpr uint32_t BLOCK_CACHE_SIZE     = 64;        ///< Блоков в LRU-кеше
static constexpr uint32_t JOURNAL_MAX_ENTRIES  = 256;

// ============================================================================
// Суперблок
// ============================================================================

/**
 * @enum FsState
 * @brief Состояние файловой системы.
 */
enum class FsState : uint8_t {
    Clean,          ///< Корректно размонтирована
    Dirty,          ///< Была смонтирована, возможен сбой
    Error           ///< Обнаружена ошибка целостности
};

/**
 * @struct Superblock
 * @brief Метаданные файловой системы (блок 0).
 *
 * Хранится в первом блоке виртуального диска.
 * Содержит все параметры геометрии ФС.
 */
struct Superblock {
    uint32_t    magic           = FS_MAGIC;
    uint32_t    version         = FS_VERSION;
    uint32_t    blockSize       = DEFAULT_BLOCK_SIZE;
    uint32_t    totalBlocks     = DEFAULT_TOTAL_BLOCKS;
    uint32_t    freeBlocks      = 0;
    uint32_t    totalInodes     = DEFAULT_MAX_INODES;
    uint32_t    freeInodes      = 0;
    uint32_t    firstDataBlock  = 0;        ///< Номер первого блока данных
    uint32_t    inodeAreaStart  = 0;        ///< Начало области инодов
    uint32_t    bitmapBlockStart = 1;       ///< Начало bitmap блоков
    uint32_t    inodeBitmapStart = 0;       ///< Начало bitmap инодов
    FsState     state           = FsState::Clean;
    uint64_t    mountCount      = 0;
    uint64_t    lastMountTick   = 0;
    uint64_t    lastWriteTick   = 0;
    std::string volumeLabel     = "RE36FS";

    /// Валиден ли суперблок?
    bool isValid() const { return magic == FS_MAGIC && version == FS_VERSION; }
};

// ============================================================================
// Сырой блок (блочное устройство)
// ============================================================================

/**
 * @struct RawBlock
 * @brief Сырой блок данных на виртуальном диске.
 */
struct RawBlock {
    uint32_t                blockId     = 0;
    std::vector<uint8_t>    data;           ///< Данные блока (blockSize байт)
    bool                    dirty       = false;    ///< Модифицирован в кеше?
};

// ============================================================================
// Журнал операций (WAL)
// ============================================================================

/**
 * @enum JournalOpType
 * @brief Тип операции в журнале.
 */
enum class JournalOpType : uint8_t {
    BlockWrite,         ///< Запись блока
    BlockFree,          ///< Освобождение блока
    InodeWrite,         ///< Запись инода
    InodeFree,          ///< Освобождение инода
    SuperblockUpdate,   ///< Обновление суперблока
    TxBegin,            ///< Начало транзакции
    TxCommit,           ///< Фиксация транзакции
    TxRollback          ///< Откат транзакции
};

/**
 * @struct JournalEntry
 * @brief Запись в журнале операций.
 */
struct JournalEntry {
    uint64_t        sequenceNum = 0;
    uint64_t        tick        = 0;
    JournalOpType   opType;
    uint32_t        targetId    = 0;        ///< blockId или inodeId
    uint32_t        txId        = 0;        ///< ID транзакции
    std::string     description;            ///< Для визуализации
};

/**
 * @struct Transaction
 * @brief Активная транзакция.
 */
struct Transaction {
    uint32_t    txId        = 0;
    uint64_t    startTick   = 0;
    bool        active      = false;
    std::vector<JournalEntry> entries;      ///< Записи транзакции
};

// ============================================================================
// Статистика драйвера
// ============================================================================

/**
 * @struct DriverStats
 * @brief Счётчики I/O и статистика кеша.
 */
struct DriverStats {
    uint64_t    totalReads      = 0;
    uint64_t    totalWrites     = 0;
    uint64_t    cacheHits       = 0;
    uint64_t    cacheMisses     = 0;
    uint64_t    blockAllocs     = 0;
    uint64_t    blockFrees      = 0;
    uint64_t    inodeAllocs     = 0;
    uint64_t    inodeFrees      = 0;
    uint64_t    journalEntries  = 0;
    uint64_t    transactions    = 0;
    uint64_t    rollbacks       = 0;
    uint32_t    cacheSize       = 0;        ///< Текущий размер кеша

    double cacheHitRate() const {
        uint64_t total = cacheHits + cacheMisses;
        return total > 0 ? static_cast<double>(cacheHits) / total * 100.0 : 0.0;
    }
};

// ============================================================================
// Запись кеша блоков (LRU)
// ============================================================================

/**
 * @struct CacheEntry
 * @brief Запись в LRU-кеше блоков.
 */
struct CacheEntry {
    RawBlock    block;
    uint64_t    lastAccess  = 0;        ///< Тик последнего доступа
    uint64_t    accessCount = 0;        ///< Количество обращений
};

// ============================================================================
// FsDriver — главный класс
// ============================================================================

/**
 * @class FsDriver
 * @brief Фундаментальный драйвер файловой системы.
 *
 * Предоставляет низкоуровневый доступ к виртуальному блочному устройству:
 * - Чтение / запись блоков (с LRU-кешем)
 * - Управление битовыми картами (блоки и иноды)
 * - Транзакционный журнал (WAL)
 * - Статистика I/O
 *
 * FileSystem делегирует сюда все блочные операции.
 * Драйвер публикует события через EventBus.
 */
class FsDriver {
public:
    /**
     * @param eventBus  Шина событий ядра
     * @param totalBlocks Количество блоков виртуального диска
     * @param blockSize  Размер одного блока (байт)
     * @param maxInodes  Максимальное количество инодов
     */
    FsDriver(EventBus& eventBus,
             uint32_t totalBlocks = DEFAULT_TOTAL_BLOCKS,
             uint32_t blockSize   = DEFAULT_BLOCK_SIZE,
             uint32_t maxInodes   = DEFAULT_MAX_INODES);
    ~FsDriver();

    FsDriver(const FsDriver&) = delete;
    FsDriver& operator=(const FsDriver&) = delete;

    // ========================================================================
    // Жизненный цикл
    // ========================================================================

    /**
     * Отформатировать виртуальный диск: создать суперблок, bitmaps,
     * обнулить все блоки.
     */
    bool format(const std::string& volumeLabel = "RE36FS");

    /**
     * Смонтировать: прочитать суперблок, загрузить bitmaps, проверить magic.
     * @return true если суперблок валиден
     */
    bool mount();

    /**
     * Размонтировать: сбросить кеш, пометить суперблок как Clean.
     */
    void unmount();

    /// Смонтирована ли ФС?
    bool isMounted() const;

    // ========================================================================
    // Блочный I/O
    // ========================================================================

    /**
     * Прочитать блок по номеру.
     * Сначала проверяет LRU-кеш, затем виртуальный диск.
     * @param blockId Номер блока
     * @return Данные блока или пустой optional
     */
    std::optional<RawBlock> readBlock(uint32_t blockId);

    /**
     * Записать блок.
     * Записывает в кеш (write-back) и помечает dirty.
     * @param blockId Номер блока
     * @param data    Данные для записи
     * @return true при успехе
     */
    bool writeBlock(uint32_t blockId, const std::vector<uint8_t>& data);

    /**
     * Записать строку как данные блока (удобство для FileSystem).
     */
    bool writeBlockString(uint32_t blockId, const std::string& content);

    /**
     * Прочитать строку из блока.
     */
    std::optional<std::string> readBlockString(uint32_t blockId);

    // ========================================================================
    // Управление блоками (bitmap)
    // ========================================================================

    /**
     * Выделить свободный блок.
     * @return Номер выделенного блока или 0 при неудаче
     */
    uint32_t allocBlock();

    /**
     * Выделить N последовательных блоков (для contiguous allocation).
     * @param count Количество блоков
     * @return Вектор номеров блоков (пустой при неудаче)
     */
    std::vector<uint32_t> allocBlocks(uint32_t count);

    /**
     * Освободить блок.
     * @param blockId Номер блока
     */
    void freeBlock(uint32_t blockId);

    /// Блок свободен?
    bool isBlockFree(uint32_t blockId) const;

    /// Количество свободных блоков
    uint32_t getFreeBlockCount() const;

    // ========================================================================
    // Управление инодами (bitmap)
    // ========================================================================

    /**
     * Выделить свободный инод.
     * @return ID инода или 0 при неудаче
     */
    uint32_t allocInode();

    /**
     * Освободить инод.
     * @param inodeId ID инода
     */
    void freeInode(uint32_t inodeId);

    /// Инод свободен?
    bool isInodeFree(uint32_t inodeId) const;

    /// Количество свободных инодов
    uint32_t getFreeInodeCount() const;

    // ========================================================================
    // Журнал (WAL)
    // ========================================================================

    /**
     * Начать транзакцию.
     * @return ID транзакции
     */
    uint32_t beginTransaction();

    /**
     * Зафиксировать транзакцию.
     * Все записи журнала с этим txId считаются подтверждёнными.
     * @param txId ID транзакции
     */
    bool commitTransaction(uint32_t txId);

    /**
     * Откатить транзакцию.
     * Записи журнала с этим txId отменяются.
     * @param txId ID транзакции
     */
    bool rollbackTransaction(uint32_t txId);

    /// Получить записи журнала
    const std::deque<JournalEntry>& getJournal() const;

    /// Очистить журнал
    void clearJournal();

    // ========================================================================
    // Кеш
    // ========================================================================

    /**
     * Сбросить все dirty-блоки из кеша на диск.
     */
    void sync();

    /**
     * Очистить кеш (без записи — данные теряются!).
     */
    void invalidateCache();

    /**
     * Установить размер кеша.
     */
    void setCacheSize(uint32_t maxEntries);

    // ========================================================================
    // Запросы
    // ========================================================================

    /// Суперблок (только чтение)
    const Superblock& getSuperblock() const;

    /// Статистика драйвера
    DriverStats getStats() const;

    /// Bitmap блоков (для визуализации)
    const std::vector<bool>& getBlockBitmap() const;

    /// Bitmap инодов (для визуализации)
    const std::vector<bool>& getInodeBitmap() const;

    /// Параметры
    uint32_t getBlockSize() const;
    uint32_t getTotalBlocks() const;
    uint32_t getMaxInodes() const;

private:
    EventBus&       eventBus_;

    // Параметры
    uint32_t        totalBlocks_;
    uint32_t        blockSize_;
    uint32_t        maxInodes_;

    // Состояние
    bool            mounted_    = false;
    Superblock      superblock_;

    // Виртуальный диск (in-memory)
    std::vector<std::vector<uint8_t>>   disk_;      ///< disk_[blockId] = данные

    // Битовые карты
    std::vector<bool>   blockBitmap_;       ///< true = занят
    std::vector<bool>   inodeBitmap_;       ///< true = занят

    // LRU-кеш блоков
    std::unordered_map<uint32_t, CacheEntry>    cache_;
    uint32_t        maxCacheSize_   = BLOCK_CACHE_SIZE;
    uint64_t        accessCounter_  = 0;

    // Журнал
    std::deque<JournalEntry>    journal_;
    uint32_t                    nextTxId_   = 1;
    uint64_t                    journalSeq_ = 0;
    std::unordered_map<uint32_t, Transaction>   activeTx_;

    // Статистика
    DriverStats     stats_;

    // Текущий тик (для журнала)
    uint64_t        currentTick_    = 0;

    // --- Внутренние методы ---

    /// Записать суперблок на диск
    void writeSuperblock();

    /// Прочитать суперблок с диска
    bool readSuperblock();

    /// Рассчитать геометрию (размеры областей)
    void calculateLayout();

    /// Вытеснить наименее используемый блок из кеша
    void evictLruCache();

    /// Записать в журнал
    void journalWrite(JournalOpType op, uint32_t targetId,
                      uint32_t txId = 0, const std::string& desc = "");

    /// Обновить тик из EventBus
    void updateTick(uint64_t tick);
};

} // namespace re36
