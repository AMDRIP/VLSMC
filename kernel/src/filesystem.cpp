// Не используется, не удалять
/**
 * @file filesystem.cpp
 * @brief Реализация виртуальной файловой системы RAND Elecorner 36.
 */

#include "kernel/filesystem.h"
#include "kernel/fs_driver.h"
#include "kernel/event_bus.h"

#include <sstream>
#include <algorithm>

namespace re36 {

FileSystem::FileSystem(EventBus& eventBus, const KernelConfig& config)
    : eventBus_(eventBus), config_(config) {}

FileSystem::~FileSystem() = default;

bool FileSystem::init() {
    totalBlocks_ = static_cast<uint32_t>(config_.diskSize / config_.blockSize);
    blocks_.resize(totalBlocks_);
    for (uint32_t i = 0; i < totalBlocks_; ++i) {
        blocks_[i].id = i;
        blocks_[i].isFree = true;
    }

    // Создать и инициализировать FsDriver
    driver_ = std::make_unique<FsDriver>(
        eventBus_, totalBlocks_,
        static_cast<uint32_t>(config_.blockSize),
        config_.maxInodes);
    driver_->format("RE36FS");
    driver_->mount();

    Inode root;
    root.id = nextInodeId_++;
    root.type = FileType::Directory;
    root.name = "/";
    root.permissions = FilePermissions::defaultDirectory();
    root.owner = ROOT_UID;
    rootInodeId_ = root.id;
    inodes_[root.id] = std::move(root);
    pathCache_["/"] = rootInodeId_;

    createSystemDirectories();
    return true;
}

bool FileSystem::loadFromFile(const std::string&) { return false; }
bool FileSystem::saveToFile(const std::string&) const { return false; }

// ---- Файлы -----------------------------------------------------------------

bool FileSystem::createFile(const std::string& path, Uid owner, FilePermissions perm) {
    std::string norm = normalizePath(path);
    if (exists(norm)) return false;

    auto [ppath, fname] = splitPath(norm);
    auto pid = resolvePath(ppath);
    if (!pid) return false;

    auto& parent = inodes_[*pid];
    if (!parent.isDirectory()) return false;

    Inode file;
    file.id = nextInodeId_++;
    file.type = FileType::RegularFile;
    file.name = fname;
    file.owner = owner;
    file.permissions = perm;
    file.parentInode = *pid;

    InodeId fid = file.id;
    inodes_[fid] = std::move(file);
    parent.children.push_back(fid);
    pathCache_[norm] = fid;

    Event evt(EventType::FileCreated, 0, "fs");
    evt.with("path", norm);
    eventBus_.publish(evt);
    return true;
}

std::optional<std::string> FileSystem::readFile(const std::string& path, Uid uid) {
    auto id = resolvePath(normalizePath(path));
    if (!id) return std::nullopt;
    auto& ino = inodes_[*id];
    if (ino.isDirectory()) return std::nullopt;
    if (!checkPermission(ino, uid, true, false, false)) return std::nullopt;
    return ino.content;
}

bool FileSystem::writeFile(const std::string& path, const std::string& content, Uid uid) {
    std::string norm = normalizePath(path);
    auto id = resolvePath(norm);
    if (!id) {
        if (!createFile(norm, uid)) return false;
        id = resolvePath(norm);
        if (!id) return false;
    }
    auto& ino = inodes_[*id];
    if (ino.isDirectory() || !checkPermission(ino, uid, false, true, false)) return false;

    freeBlocks(ino.dataBlocks);
    ino.dataBlocks = allocateBlocks(content.size());
    ino.content = content;
    ino.size = content.size();

    Event evt(EventType::FileModified, 0, "fs");
    evt.with("path", norm).with("size", static_cast<int64_t>(content.size()));
    eventBus_.publish(evt);
    return true;
}

bool FileSystem::deleteFile(const std::string& path, Uid uid) {
    std::string norm = normalizePath(path);
    auto id = resolvePath(norm);
    if (!id) return false;
    auto& ino = inodes_[*id];
    if (ino.isDirectory()) return false;
    if (!checkPermission(ino, uid, false, true, false) && uid != ROOT_UID) return false;

    auto pit = inodes_.find(ino.parentInode);
    if (pit != inodes_.end()) {
        auto& ch = pit->second.children;
        ch.erase(std::remove(ch.begin(), ch.end(), *id), ch.end());
    }
    freeBlocks(ino.dataBlocks);
    inodes_.erase(*id);
    invalidatePathCache();
    return true;
}

bool FileSystem::rename(const std::string& oldP, const std::string& newP, Uid uid) {
    std::string no = normalizePath(oldP), nn = normalizePath(newP);
    auto id = resolvePath(no);
    if (!id || exists(nn)) return false;
    auto& ino = inodes_[*id];
    if (!checkPermission(ino, uid, false, true, false) && uid != ROOT_UID) return false;

    auto [npPath, nName] = splitPath(nn);
    auto npId = resolvePath(npPath);
    if (!npId) return false;

    auto opit = inodes_.find(ino.parentInode);
    if (opit != inodes_.end()) {
        auto& ch = opit->second.children;
        ch.erase(std::remove(ch.begin(), ch.end(), *id), ch.end());
    }
    inodes_[*npId].children.push_back(*id);
    ino.name = nName;
    ino.parentInode = *npId;
    invalidatePathCache();
    return true;
}

bool FileSystem::copyFile(const std::string& s, const std::string& d, Uid uid) {
    auto c = readFile(s, uid);
    if (!c) return false;
    if (!createFile(d, uid)) return false;
    return writeFile(d, *c, uid);
}

std::optional<FileStat> FileSystem::stat(const std::string& path) const {
    auto id = resolvePath(normalizePath(path));
    if (!id) return std::nullopt;
    auto it = inodes_.find(*id);
    if (it == inodes_.end()) return std::nullopt;
    const auto& i = it->second;
    return FileStat{i.name, normalizePath(path), i.type, i.size,
                    i.permissions.toString(), std::to_string(i.owner),
                    i.createdAt, i.modifiedAt,
                    static_cast<uint32_t>(i.dataBlocks.size()), i.id};
}

bool FileSystem::exists(const std::string& path) const {
    return resolvePath(normalizePath(path)).has_value();
}

// ---- Каталоги --------------------------------------------------------------

bool FileSystem::makeDirectory(const std::string& path, Uid owner, FilePermissions perm) {
    std::string norm = normalizePath(path);
    if (exists(norm)) return false;
    auto [pp, dn] = splitPath(norm);
    auto pid = resolvePath(pp);
    if (!pid || !inodes_[*pid].isDirectory()) return false;

    Inode dir;
    dir.id = nextInodeId_++;
    dir.type = FileType::Directory;
    dir.name = dn;
    dir.owner = owner;
    dir.permissions = perm;
    dir.parentInode = *pid;
    InodeId did = dir.id;
    inodes_[did] = std::move(dir);
    inodes_[*pid].children.push_back(did);
    pathCache_[norm] = did;
    return true;
}

bool FileSystem::removeDirectory(const std::string& path, Uid uid) {
    std::string norm = normalizePath(path);
    if (norm == "/") return false;
    auto id = resolvePath(norm);
    if (!id) return false;
    auto it = inodes_.find(*id);
    if (it == inodes_.end() || !it->second.isDirectory() || !it->second.children.empty()) return false;
    if (!checkPermission(it->second, uid, false, true, false) && uid != ROOT_UID) return false;

    auto pit = inodes_.find(it->second.parentInode);
    if (pit != inodes_.end()) {
        auto& ch = pit->second.children;
        ch.erase(std::remove(ch.begin(), ch.end(), *id), ch.end());
    }
    inodes_.erase(*id);
    invalidatePathCache();
    return true;
}

std::optional<std::vector<DirectoryEntry>> FileSystem::listDirectory(
        const std::string& path, Uid uid) const {
    auto id = resolvePath(normalizePath(path));
    if (!id) return std::nullopt;
    auto it = inodes_.find(*id);
    if (it == inodes_.end() || !it->second.isDirectory()) return std::nullopt;
    if (!checkPermission(it->second, uid, true, false, false) && uid != ROOT_UID) return std::nullopt;

    std::vector<DirectoryEntry> entries;
    for (InodeId cid : it->second.children) {
        auto ci = inodes_.find(cid);
        if (ci != inodes_.end()) {
            entries.push_back({ci->second.name, ci->second.type, ci->second.size,
                               ci->second.permissions.toString(),
                               std::to_string(ci->second.owner), ci->second.modifiedAt});
        }
    }
    return entries;
}

// ---- Права -----------------------------------------------------------------

bool FileSystem::changePermissions(const std::string& path, FilePermissions p, Uid uid) {
    auto id = resolvePath(normalizePath(path));
    if (!id) return false;
    auto& ino = inodes_[*id];
    if (ino.owner != uid && uid != ROOT_UID) return false;
    ino.permissions = p;
    return true;
}

bool FileSystem::changeOwner(const std::string& path, Uid newOwner, Uid uid) {
    if (uid != ROOT_UID) return false;
    auto id = resolvePath(normalizePath(path));
    if (!id) return false;
    inodes_[*id].owner = newOwner;
    return true;
}

void FileSystem::setBlockAllocationMethod(BlockAllocationMethod m) { blockMethod_ = m; }
BlockAllocationMethod FileSystem::getBlockAllocationMethod() const { return blockMethod_; }

// ---- Визуализация ----------------------------------------------------------

DiskStats FileSystem::getDiskStats() const {
    DiskStats s;
    s.totalSize = config_.diskSize;
    s.totalBlocks = totalBlocks_;
    s.totalInodes = config_.maxInodes;
    uint32_t ub = 0;
    for (auto& b : blocks_) if (!b.isFree) ub++;
    s.usedBlocks = ub;
    s.freeBlocks = totalBlocks_ - ub;
    s.usedSize = static_cast<size_t>(ub) * config_.blockSize;
    s.freeSize = s.totalSize - s.usedSize;
    s.usedInodes = static_cast<uint32_t>(inodes_.size());
    s.freeInodes = config_.maxInodes - s.usedInodes;
    s.usagePercent = totalBlocks_ > 0 ? static_cast<double>(ub) / totalBlocks_ * 100.0 : 0.0;

    // Дополнить данными из драйвера (если доступен)
    if (driver_ && driver_->isMounted()) {
        const auto& sb = driver_->getSuperblock();
        s.freeBlocks = sb.freeBlocks;
        s.usedBlocks = sb.totalBlocks - sb.freeBlocks;
    }

    return s;
}

std::vector<DiskBlock> FileSystem::getBlockMap() const { return blocks_; }

std::vector<Inode> FileSystem::getInodeTable() const {
    std::vector<Inode> r;
    for (auto& [id, ino] : inodes_) r.push_back(ino);
    return r;
}

FileSystem::TreeNode FileSystem::getDirectoryTree(const std::string& rootPath) const {
    TreeNode node;
    auto id = resolvePath(normalizePath(rootPath));
    if (!id) return node;
    auto it = inodes_.find(*id);
    if (it == inodes_.end()) return node;
    node.name = it->second.name;
    node.type = it->second.type;
    node.size = it->second.size;
    if (it->second.isDirectory()) {
        for (InodeId cid : it->second.children) {
            auto ci = inodes_.find(cid);
            if (ci != inodes_.end()) {
                std::string cp = rootPath;
                if (cp.back() != '/') cp += '/';
                cp += ci->second.name;
                node.children.push_back(getDirectoryTree(cp));
            }
        }
    }
    return node;
}

// ---- Внутренние ------------------------------------------------------------

std::optional<InodeId> FileSystem::resolvePath(const std::string& path) const {
    auto ci = pathCache_.find(path);
    if (ci != pathCache_.end() && inodes_.count(ci->second)) return ci->second;
    if (path == "/" || path.empty()) return rootInodeId_;

    std::vector<std::string> parts;
    std::istringstream iss(path);
    std::string p;
    while (std::getline(iss, p, '/')) if (!p.empty() && p != ".") parts.push_back(p);

    InodeId cur = rootInodeId_;
    for (auto& name : parts) {
        auto it = inodes_.find(cur);
        if (it == inodes_.end() || !it->second.isDirectory()) return std::nullopt;
        bool found = false;
        for (InodeId cid : it->second.children) {
            auto ci2 = inodes_.find(cid);
            if (ci2 != inodes_.end() && ci2->second.name == name) { cur = cid; found = true; break; }
        }
        if (!found) return std::nullopt;
    }
    pathCache_[path] = cur;
    return cur;
}

std::string FileSystem::normalizePath(const std::string& path) const {
    if (path.empty()) return "/";
    std::istringstream iss(path);
    std::string p;
    std::vector<std::string> parts;
    while (std::getline(iss, p, '/')) {
        if (p.empty() || p == ".") continue;
        if (p == "..") { if (!parts.empty()) parts.pop_back(); }
        else parts.push_back(p);
    }
    if (parts.empty()) return "/";
    std::string r;
    for (auto& x : parts) r += "/" + x;
    return r;
}

std::pair<std::string, std::string> FileSystem::splitPath(const std::string& path) const {
    auto pos = path.rfind('/');
    if (pos == std::string::npos || pos == 0) return {"/", path.substr(pos == 0 ? 1 : 0)};
    return {path.substr(0, pos), path.substr(pos + 1)};
}

std::vector<BlockId> FileSystem::allocateBlocks(size_t dataSize) {
    uint32_t needed = std::max<uint32_t>(1, static_cast<uint32_t>((dataSize + config_.blockSize - 1) / config_.blockSize));

    // Делегировать драйверу если доступен
    if (driver_ && driver_->isMounted()) {
        auto driverBlocks = driver_->allocBlocks(needed);
        std::vector<BlockId> alloc;
        alloc.reserve(driverBlocks.size());
        for (auto bid : driverBlocks) {
            alloc.push_back(static_cast<BlockId>(bid));
            // Синхронизировать legacy-массив
            if (bid < totalBlocks_) blocks_[bid].isFree = false;
        }
        return alloc;
    }

    // Fallback: legacy-алгоритм
    std::vector<BlockId> alloc;
    for (uint32_t i = 0; i < totalBlocks_ && alloc.size() < needed; ++i)
        if (blocks_[i].isFree) { blocks_[i].isFree = false; alloc.push_back(i); }
    return alloc;
}

void FileSystem::freeBlocks(const std::vector<BlockId>& ids) {
    for (auto id : ids) {
        if (id < totalBlocks_) {
            blocks_[id].isFree = true;
            blocks_[id].data.clear();
        }
        // Делегировать драйверу
        if (driver_ && driver_->isMounted()) {
            driver_->freeBlock(static_cast<uint32_t>(id));
        }
    }
}

bool FileSystem::checkPermission(const Inode& ino, Uid uid, bool r, bool w, bool x) const {
    if (uid == ROOT_UID) return true;
    const auto& p = ino.permissions;
    if (uid == ino.owner) {
        if (r && !p.ownerRead) return false;
        if (w && !p.ownerWrite) return false;
        if (x && !p.ownerExecute) return false;
    } else {
        if (r && !p.otherRead) return false;
        if (w && !p.otherWrite) return false;
        if (x && !p.otherExecute) return false;
    }
    return true;
}

void FileSystem::createSystemDirectories() {
    makeDirectory("/system", ROOT_UID);
    makeDirectory("/home", ROOT_UID);
    makeDirectory("/apps", ROOT_UID);
    makeDirectory("/tmp", ROOT_UID);
    makeDirectory("/home/root", ROOT_UID);
}

void FileSystem::invalidatePathCache() {
    pathCache_.clear();
    pathCache_["/"] = rootInodeId_;
}

} // namespace re36
