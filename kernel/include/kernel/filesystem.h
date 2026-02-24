/**
 * @file filesystem.h
 * @brief Файловая система RAND Elecorner 36.
 *
 * Иерархическая файловая система с инодами, каталогами, блоками,
 * правами доступа и персистентностью.
 * Соответствует требованиям FS-01 — FS-08.
 */

#pragma once

#include "types.h"
#include "fs_driver.h"

#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <memory>

namespace re36 {

// Предварительные объявления
class EventBus;

// ============================================================================
// Типы файловой системы
// ============================================================================

/**
 * @enum FileType
 * @brief Тип объекта файловой системы.
 */
enum class FileType : uint8_t {
    RegularFile,
    Directory,
    SymLink
};

/**
 * @enum BlockAllocationMethod
 * @brief Метод размещения блоков на диске (FS-06).
 */
enum class BlockAllocationMethod : uint8_t {
    Contiguous,     ///< Непрерывное
    Linked,         ///< Связанное (цепочка)
    Indexed         ///< Индексное
};

// ============================================================================
// Инод (FS-02)
// ============================================================================

/**
 * @struct Inode
 * @brief Метаданные файла или каталога.
 */
struct Inode {
    InodeId             id              = 0;
    FileType            type            = FileType::RegularFile;
    std::string         name;
    size_t              size            = 0;        ///< Размер данных (байт)
    FilePermissions     permissions     = FilePermissions::defaultFile();
    Uid                 owner           = ROOT_UID;
    Gid                 group           = 0;
    Tick                createdAt       = 0;
    Tick                modifiedAt      = 0;
    Tick                accessedAt      = 0;

    // Блоки на диске
    std::vector<BlockId> dataBlocks;                 ///< Прямые указатели на блоки
    BlockId             indexBlock      = 0;         ///< Индексный блок (для Indexed)

    // Дерево каталогов
    InodeId             parentInode     = 0;
    std::vector<InodeId> children;                   ///< Дочерние иноды (для каталогов)

    // Файлы
    std::string         content;                     ///< Содержимое файла (для простоты — строка)

    /// Это каталог?
    bool isDirectory() const { return type == FileType::Directory; }
};

// ============================================================================
// Блок на «диске»
// ============================================================================

/**
 * @struct DiskBlock
 * @brief Один блок на виртуальном диске.
 */
struct DiskBlock {
    BlockId             id          = 0;
    bool                isFree      = true;
    InodeId             ownerInode  = 0;         ///< Инод-владелец
    BlockId             nextBlock   = 0;         ///< Следующий блок (для Linked)
    std::vector<uint8_t> data;                   ///< Данные блока
};

// ============================================================================
// Статистика диска
// ============================================================================

/**
 * @struct DiskStats
 * @brief Статистика виртуального диска.
 */
struct DiskStats {
    size_t      totalSize       = 0;
    size_t      usedSize        = 0;
    size_t      freeSize        = 0;
    uint32_t    totalBlocks     = 0;
    uint32_t    usedBlocks      = 0;
    uint32_t    freeBlocks      = 0;
    uint32_t    totalInodes     = 0;
    uint32_t    usedInodes      = 0;
    uint32_t    freeInodes      = 0;
    double      usagePercent    = 0.0;
};

// ============================================================================
// Информация о файле (для GUI и команды stat)
// ============================================================================

/**
 * @struct FileStat
 * @brief Публичная информация о файле / каталоге.
 */
struct FileStat {
    std::string         name;
    std::string         fullPath;
    FileType            type;
    size_t              size;
    std::string         permissions;    ///< "rwxr-xr-x"
    std::string         owner;
    Tick                createdAt;
    Tick                modifiedAt;
    uint32_t            blockCount;
    InodeId             inodeId;
};

// ============================================================================
// Запись каталога (для ls)
// ============================================================================

/**
 * @struct DirectoryEntry
 * @brief Одна запись при выводе содержимого каталога.
 */
struct DirectoryEntry {
    std::string     name;
    FileType        type;
    size_t          size;
    std::string     permissions;
    std::string     owner;
    Tick            modifiedAt;
};

// ============================================================================
// Файловая система
// ============================================================================

/**
 * @class FileSystem
 * @brief Виртуальная файловая система.
 */
class FileSystem {
public:
    explicit FileSystem(EventBus& eventBus, const KernelConfig& config);
    ~FileSystem();

    FileSystem(const FileSystem&) = delete;
    FileSystem& operator=(const FileSystem&) = delete;

    // ========================================================================
    // Инициализация
    // ========================================================================

    /**
     * Инициализировать ФС: создать диск, корневой каталог,
     * системные каталоги (/system, /home, /apps, /tmp).
     */
    bool init();

    /**
     * Загрузить состояние ФС из файла на хосте (FS-08).
     * @param path Путь к файлу на хост-системе
     * @return true при успешной загрузке
     */
    bool loadFromFile(const std::string& path);

    /**
     * Сохранить состояние ФС в файл на хосте (FS-08).
     * @param path Путь к файлу на хост-системе
     * @return true при успешном сохранении
     */
    bool saveToFile(const std::string& path) const;

    // ========================================================================
    // Операции с файлами (FS-03)
    // ========================================================================

    /**
     * Создать файл.
     * @param path        Полный путь
     * @param owner       UID владельца
     * @param permissions Права доступа
     * @return true при успехе
     */
    bool createFile(const std::string& path, Uid owner = ROOT_UID,
                    FilePermissions permissions = FilePermissions::defaultFile());

    /// Прочитать содержимое файла
    std::optional<std::string> readFile(const std::string& path, Uid requestor = ROOT_UID);

    /// Записать данные в файл
    bool writeFile(const std::string& path, const std::string& content, Uid requestor = ROOT_UID);

    /// Удалить файл
    bool deleteFile(const std::string& path, Uid requestor = ROOT_UID);

    /// Переименовать файл или каталог
    bool rename(const std::string& oldPath, const std::string& newPath, Uid requestor = ROOT_UID);

    /// Копировать файл
    bool copyFile(const std::string& src, const std::string& dst, Uid requestor = ROOT_UID);

    /// Получить метаданные (stat)
    std::optional<FileStat> stat(const std::string& path) const;

    /// Проверить существование
    bool exists(const std::string& path) const;

    // ========================================================================
    // Операции с каталогами (FS-04)
    // ========================================================================

    /// Создать каталог
    bool makeDirectory(const std::string& path, Uid owner = ROOT_UID,
                       FilePermissions permissions = FilePermissions::defaultDirectory());

    /// Удалить каталог (должен быть пустым)
    bool removeDirectory(const std::string& path, Uid requestor = ROOT_UID);

    /// Содержимое каталога
    std::optional<std::vector<DirectoryEntry>> listDirectory(const std::string& path,
                                                              Uid requestor = ROOT_UID) const;

    // ========================================================================
    // Права доступа (FS-05)
    // ========================================================================

    /// Изменить права
    bool changePermissions(const std::string& path, FilePermissions perms, Uid requestor = ROOT_UID);

    /// Изменить владельца
    bool changeOwner(const std::string& path, Uid newOwner, Uid requestor = ROOT_UID);

    // ========================================================================
    // Настройки
    // ========================================================================

    /// Метод размещения блоков (FS-06)
    void setBlockAllocationMethod(BlockAllocationMethod method);
    BlockAllocationMethod getBlockAllocationMethod() const;

    // ========================================================================
    // Визуализация и запросы
    // ========================================================================

    /// Статистика диска
    DiskStats getDiskStats() const;

    /// Карта блоков (для визуализации)
    std::vector<DiskBlock> getBlockMap() const;

    /// Драйвер файловой системы (для визуализации и мониторинга)
    FsDriver* getDriver() { return driver_.get(); }
    const FsDriver* getDriver() const { return driver_.get(); }

    /// Список всех инодов (для таблицы инодов)
    std::vector<Inode> getInodeTable() const;

    /// Дерево каталогов (рекурсивное)
    struct TreeNode {
        std::string name;
        FileType    type;
        size_t      size;
        std::vector<TreeNode> children;
    };
    TreeNode getDirectoryTree(const std::string& rootPath = "/") const;

private:
    EventBus&                                   eventBus_;
    KernelConfig                                config_;

    // Иноды
    std::unordered_map<InodeId, Inode>          inodes_;
    InodeId                                     nextInodeId_ = 1;
    InodeId                                     rootInodeId_ = 0;

    // Блоки на «диске»
    std::vector<DiskBlock>                      blocks_;
    uint32_t                                    totalBlocks_ = 0;

    // Кеш путей (путь → InodeId)
    mutable std::unordered_map<std::string, InodeId> pathCache_;

    // Метод размещения
    BlockAllocationMethod                       blockMethod_ = BlockAllocationMethod::Indexed;

    // Драйвер
    std::unique_ptr<FsDriver>                   driver_;

    // --- Внутренние методы ---

    /// Разрешить путь в InodeId
    std::optional<InodeId> resolvePath(const std::string& path) const;

    /// Нормализовать путь (убрать //, /./, разрешить /../)
    std::string normalizePath(const std::string& path) const;

    /// Получить родительский путь и имя файла
    std::pair<std::string, std::string> splitPath(const std::string& path) const;

    /// Выделить блоки для данных
    std::vector<BlockId> allocateBlocks(size_t dataSize);

    /// Освободить блоки
    void freeBlocks(const std::vector<BlockId>& blocks);

    /// Проверить права доступа
    bool checkPermission(const Inode& inode, Uid requestor, bool read, bool write, bool execute) const;

    /// Создать системные каталоги
    void createSystemDirectories();

    /// Инвалидировать кеш путей
    void invalidatePathCache();
};

} // namespace re36
