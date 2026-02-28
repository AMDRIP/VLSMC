


#include "kernel/fs_driver.h"
#include "kernel/event_bus.h"

#include <algorithm>
#include <cstring>

namespace re36 {





FsDriver::FsDriver(EventBus& eventBus, uint32_t totalBlocks,
                   uint32_t blockSize, uint32_t maxInodes)
    : eventBus_(eventBus)
    , totalBlocks_(totalBlocks)
    , blockSize_(blockSize)
    , maxInodes_(maxInodes) {}

FsDriver::~FsDriver() {
    if (mounted_) {
        unmount();
    }
}





bool FsDriver::format(const std::string& volumeLabel) {
    
    disk_.resize(totalBlocks_);
    for (uint32_t i = 0; i < totalBlocks_; ++i) {
        disk_[i].resize(blockSize_, 0);
    }

    
    calculateLayout();

    
    superblock_ = Superblock{};
    superblock_.magic = FS_MAGIC;
    superblock_.version = FS_VERSION;
    superblock_.blockSize = blockSize_;
    superblock_.totalBlocks = totalBlocks_;
    superblock_.totalInodes = maxInodes_;
    superblock_.volumeLabel = volumeLabel;
    superblock_.state = FsState::Clean;
    superblock_.mountCount = 0;

    
    blockBitmap_.assign(totalBlocks_, false);
    inodeBitmap_.assign(maxInodes_, false);

    
    uint32_t reservedBlocks = superblock_.firstDataBlock;
    for (uint32_t i = 0; i < reservedBlocks && i < totalBlocks_; ++i) {
        blockBitmap_[i] = true;   
    }

    superblock_.freeBlocks = totalBlocks_ - reservedBlocks;
    superblock_.freeInodes = maxInodes_;

    
    writeSuperblock();

    
    journal_.clear();
    cache_.clear();
    stats_ = DriverStats{};
    journalSeq_ = 0;
    nextTxId_ = 1;
    activeTx_.clear();

    
    Event evt(EventType::FileCreated, 0, "fs_driver");
    evt.with("action", std::string("format"))
       .with("volumeLabel", volumeLabel)
       .with("totalBlocks", static_cast<int64_t>(totalBlocks_))
       .with("blockSize", static_cast<int64_t>(blockSize_));
    eventBus_.publish(evt);

    return true;
}

bool FsDriver::mount() {
    if (mounted_) return false;

    
    if (disk_.empty()) return false;

    
    if (!readSuperblock()) return false;

    
    if (!superblock_.isValid()) return false;

    
    superblock_.state = FsState::Dirty;
    superblock_.mountCount++;
    writeSuperblock();

    mounted_ = true;
    return true;
}

void FsDriver::unmount() {
    if (!mounted_) return;

    
    sync();

    
    superblock_.state = FsState::Clean;
    writeSuperblock();

    mounted_ = false;
}

bool FsDriver::isMounted() const {
    return mounted_;
}





std::optional<RawBlock> FsDriver::readBlock(uint32_t blockId) {
    if (blockId >= totalBlocks_) return std::nullopt;

    
    auto cacheIt = cache_.find(blockId);
    if (cacheIt != cache_.end()) {
        stats_.cacheHits++;
        cacheIt->second.lastAccess = ++accessCounter_;
        cacheIt->second.accessCount++;
        return cacheIt->second.block;
    }

    
    stats_.cacheMisses++;
    stats_.totalReads++;

    RawBlock block;
    block.blockId = blockId;
    block.data = disk_[blockId];
    block.dirty = false;

    
    if (cache_.size() >= maxCacheSize_) {
        evictLruCache();
    }

    CacheEntry entry;
    entry.block = block;
    entry.lastAccess = ++accessCounter_;
    entry.accessCount = 1;
    cache_[blockId] = std::move(entry);

    return block;
}

bool FsDriver::writeBlock(uint32_t blockId, const std::vector<uint8_t>& data) {
    if (blockId >= totalBlocks_) return false;

    stats_.totalWrites++;

    
    std::vector<uint8_t> padded(blockSize_, 0);
    size_t copySize = std::min(data.size(), static_cast<size_t>(blockSize_));
    std::copy_n(data.begin(), copySize, padded.begin());

    
    auto cacheIt = cache_.find(blockId);
    if (cacheIt != cache_.end()) {
        cacheIt->second.block.data = padded;
        cacheIt->second.block.dirty = true;
        cacheIt->second.lastAccess = ++accessCounter_;
        cacheIt->second.accessCount++;
    } else {
        if (cache_.size() >= maxCacheSize_) {
            evictLruCache();
        }

        CacheEntry entry;
        entry.block.blockId = blockId;
        entry.block.data = padded;
        entry.block.dirty = true;
        entry.lastAccess = ++accessCounter_;
        entry.accessCount = 1;
        cache_[blockId] = std::move(entry);
    }

    
    disk_[blockId] = padded;

    superblock_.lastWriteTick = currentTick_;

    return true;
}

bool FsDriver::writeBlockString(uint32_t blockId, const std::string& content) {
    std::vector<uint8_t> data(content.begin(), content.end());
    return writeBlock(blockId, data);
}

std::optional<std::string> FsDriver::readBlockString(uint32_t blockId) {
    auto block = readBlock(blockId);
    if (!block) return std::nullopt;

    
    auto it = std::find(block->data.begin(), block->data.end(), 0);
    size_t len = static_cast<size_t>(std::distance(block->data.begin(), it));
    return std::string(block->data.begin(), block->data.begin() + len);
}





uint32_t FsDriver::allocBlock() {
    
    uint32_t start = superblock_.firstDataBlock;
    for (uint32_t i = start; i < totalBlocks_; ++i) {
        if (!blockBitmap_[i]) {
            blockBitmap_[i] = true;
            superblock_.freeBlocks--;
            stats_.blockAllocs++;

            
            disk_[i].assign(blockSize_, 0);

            
            journalWrite(JournalOpType::BlockWrite, i, 0, "allocBlock");

            return i;
        }
    }
    return 0;   
}

std::vector<uint32_t> FsDriver::allocBlocks(uint32_t count) {
    if (count == 0) return {};

    std::vector<uint32_t> result;
    result.reserve(count);

    for (uint32_t i = 0; i < count; ++i) {
        uint32_t blk = allocBlock();
        if (blk == 0) {
            
            for (auto id : result) freeBlock(id);
            return {};
        }
        result.push_back(blk);
    }
    return result;
}

void FsDriver::freeBlock(uint32_t blockId) {
    if (blockId >= totalBlocks_) return;
    if (blockId < superblock_.firstDataBlock) return;   
    if (!blockBitmap_[blockId]) return;                 

    blockBitmap_[blockId] = false;
    superblock_.freeBlocks++;
    stats_.blockFrees++;

    
    disk_[blockId].assign(blockSize_, 0);

    
    cache_.erase(blockId);

    
    journalWrite(JournalOpType::BlockFree, blockId, 0, "freeBlock");
}

bool FsDriver::isBlockFree(uint32_t blockId) const {
    if (blockId >= totalBlocks_) return false;
    return !blockBitmap_[blockId];
}

uint32_t FsDriver::getFreeBlockCount() const {
    return superblock_.freeBlocks;
}





uint32_t FsDriver::allocInode() {
    for (uint32_t i = 1; i < maxInodes_; ++i) {    
        if (!inodeBitmap_[i]) {
            inodeBitmap_[i] = true;
            superblock_.freeInodes--;
            stats_.inodeAllocs++;

            journalWrite(JournalOpType::InodeWrite, i, 0, "allocInode");
            return i;
        }
    }
    return 0;   
}

void FsDriver::freeInode(uint32_t inodeId) {
    if (inodeId == 0 || inodeId >= maxInodes_) return;
    if (!inodeBitmap_[inodeId]) return;     

    inodeBitmap_[inodeId] = false;
    superblock_.freeInodes++;
    stats_.inodeFrees++;

    journalWrite(JournalOpType::InodeFree, inodeId, 0, "freeInode");
}

bool FsDriver::isInodeFree(uint32_t inodeId) const {
    if (inodeId >= maxInodes_) return false;
    return !inodeBitmap_[inodeId];
}

uint32_t FsDriver::getFreeInodeCount() const {
    return superblock_.freeInodes;
}





uint32_t FsDriver::beginTransaction() {
    uint32_t txId = nextTxId_++;

    Transaction tx;
    tx.txId = txId;
    tx.startTick = currentTick_;
    tx.active = true;
    activeTx_[txId] = std::move(tx);

    stats_.transactions++;

    journalWrite(JournalOpType::TxBegin, 0, txId, "beginTx");

    return txId;
}

bool FsDriver::commitTransaction(uint32_t txId) {
    auto it = activeTx_.find(txId);
    if (it == activeTx_.end() || !it->second.active) return false;

    it->second.active = false;

    journalWrite(JournalOpType::TxCommit, 0, txId, "commitTx");

    activeTx_.erase(it);
    return true;
}

bool FsDriver::rollbackTransaction(uint32_t txId) {
    auto it = activeTx_.find(txId);
    if (it == activeTx_.end() || !it->second.active) return false;

    it->second.active = false;
    stats_.rollbacks++;

    journalWrite(JournalOpType::TxRollback, 0, txId, "rollbackTx");

    activeTx_.erase(it);
    return true;
}

const std::deque<JournalEntry>& FsDriver::getJournal() const {
    return journal_;
}

void FsDriver::clearJournal() {
    journal_.clear();
}





void FsDriver::sync() {
    for (auto& [blockId, entry] : cache_) {
        if (entry.block.dirty) {
            disk_[blockId] = entry.block.data;
            entry.block.dirty = false;
        }
    }

    
    writeSuperblock();
}

void FsDriver::invalidateCache() {
    cache_.clear();
    stats_.cacheSize = 0;
}

void FsDriver::setCacheSize(uint32_t maxEntries) {
    maxCacheSize_ = maxEntries;
    while (cache_.size() > maxCacheSize_) {
        evictLruCache();
    }
}





const Superblock& FsDriver::getSuperblock() const {
    return superblock_;
}

DriverStats FsDriver::getStats() const {
    DriverStats s = stats_;
    s.cacheSize = static_cast<uint32_t>(cache_.size());
    return s;
}

const std::vector<bool>& FsDriver::getBlockBitmap() const {
    return blockBitmap_;
}

const std::vector<bool>& FsDriver::getInodeBitmap() const {
    return inodeBitmap_;
}

uint32_t FsDriver::getBlockSize() const { return blockSize_; }
uint32_t FsDriver::getTotalBlocks() const { return totalBlocks_; }
uint32_t FsDriver::getMaxInodes() const { return maxInodes_; }





void FsDriver::writeSuperblock() {
    
    std::vector<uint8_t> data(blockSize_, 0);

    
    auto writeU32 = [&data](size_t offset, uint32_t val) {
        if (offset + 4 <= data.size()) {
            data[offset + 0] = static_cast<uint8_t>(val & 0xFF);
            data[offset + 1] = static_cast<uint8_t>((val >> 8) & 0xFF);
            data[offset + 2] = static_cast<uint8_t>((val >> 16) & 0xFF);
            data[offset + 3] = static_cast<uint8_t>((val >> 24) & 0xFF);
        }
    };

    auto writeU64 = [&data](size_t offset, uint64_t val) {
        for (int i = 0; i < 8; ++i) {
            if (offset + i < data.size()) {
                data[offset + i] = static_cast<uint8_t>((val >> (i * 8)) & 0xFF);
            }
        }
    };

    size_t off = 0;
    writeU32(off, superblock_.magic);           off += 4;
    writeU32(off, superblock_.version);         off += 4;
    writeU32(off, superblock_.blockSize);       off += 4;
    writeU32(off, superblock_.totalBlocks);     off += 4;
    writeU32(off, superblock_.freeBlocks);      off += 4;
    writeU32(off, superblock_.totalInodes);     off += 4;
    writeU32(off, superblock_.freeInodes);      off += 4;
    writeU32(off, superblock_.firstDataBlock);  off += 4;
    writeU32(off, superblock_.inodeAreaStart);  off += 4;
    writeU32(off, superblock_.bitmapBlockStart); off += 4;
    writeU32(off, superblock_.inodeBitmapStart); off += 4;
    data[off++] = static_cast<uint8_t>(superblock_.state);
    
    off = 48;
    writeU64(off, superblock_.mountCount);      off += 8;
    writeU64(off, superblock_.lastMountTick);   off += 8;
    writeU64(off, superblock_.lastWriteTick);   off += 8;

    
    off = 72;
    size_t lblLen = std::min(superblock_.volumeLabel.size(), static_cast<size_t>(31));
    for (size_t i = 0; i < lblLen; ++i) {
        data[off + i] = static_cast<uint8_t>(superblock_.volumeLabel[i]);
    }

    if (!disk_.empty()) {
        disk_[0] = data;
    }

    journalWrite(JournalOpType::SuperblockUpdate, 0, 0, "writeSuperblock");
}

bool FsDriver::readSuperblock() {
    if (disk_.empty() || disk_[0].size() < 48) return false;

    const auto& data = disk_[0];

    auto readU32 = [&data](size_t offset) -> uint32_t {
        if (offset + 4 > data.size()) return 0;
        return static_cast<uint32_t>(data[offset])
             | (static_cast<uint32_t>(data[offset + 1]) << 8)
             | (static_cast<uint32_t>(data[offset + 2]) << 16)
             | (static_cast<uint32_t>(data[offset + 3]) << 24);
    };

    auto readU64 = [&data](size_t offset) -> uint64_t {
        uint64_t val = 0;
        for (int i = 0; i < 8; ++i) {
            if (offset + i < data.size()) {
                val |= static_cast<uint64_t>(data[offset + i]) << (i * 8);
            }
        }
        return val;
    };

    size_t off = 0;
    superblock_.magic = readU32(off);             off += 4;
    superblock_.version = readU32(off);           off += 4;
    superblock_.blockSize = readU32(off);         off += 4;
    superblock_.totalBlocks = readU32(off);       off += 4;
    superblock_.freeBlocks = readU32(off);        off += 4;
    superblock_.totalInodes = readU32(off);       off += 4;
    superblock_.freeInodes = readU32(off);        off += 4;
    superblock_.firstDataBlock = readU32(off);    off += 4;
    superblock_.inodeAreaStart = readU32(off);    off += 4;
    superblock_.bitmapBlockStart = readU32(off);  off += 4;
    superblock_.inodeBitmapStart = readU32(off);  off += 4;
    superblock_.state = static_cast<FsState>(data[off++]);

    off = 48;
    superblock_.mountCount = readU64(off);        off += 8;
    superblock_.lastMountTick = readU64(off);     off += 8;
    superblock_.lastWriteTick = readU64(off);     off += 8;

    
    off = 72;
    std::string label;
    for (size_t i = 0; i < 31 && off + i < data.size() && data[off + i] != 0; ++i) {
        label += static_cast<char>(data[off + i]);
    }
    superblock_.volumeLabel = label;

    return superblock_.isValid();
}

void FsDriver::calculateLayout() {
    
    
    
    
    

    
    
    uint32_t bitsPerBlock = blockSize_ * 8;
    uint32_t blockBitmapBlocks = (totalBlocks_ + bitsPerBlock - 1) / bitsPerBlock;

    
    uint32_t inodeBitmapBlocks = (maxInodes_ + bitsPerBlock - 1) / bitsPerBlock;

    superblock_.bitmapBlockStart = 1;
    superblock_.inodeBitmapStart = superblock_.bitmapBlockStart + blockBitmapBlocks;
    superblock_.inodeAreaStart = superblock_.inodeBitmapStart + inodeBitmapBlocks;

    
    
    uint32_t inodeBlocks = (maxInodes_ * 128 + blockSize_ - 1) / blockSize_;  

    superblock_.firstDataBlock = superblock_.inodeAreaStart + inodeBlocks;

    
    if (superblock_.firstDataBlock >= totalBlocks_) {
        superblock_.firstDataBlock = totalBlocks_ / 2;  
    }
}

void FsDriver::evictLruCache() {
    if (cache_.empty()) return;

    
    uint32_t victimId = 0;
    uint64_t minAccess = UINT64_MAX;

    for (auto& [blockId, entry] : cache_) {
        if (entry.lastAccess < minAccess) {
            minAccess = entry.lastAccess;
            victimId = blockId;
        }
    }

    
    auto it = cache_.find(victimId);
    if (it != cache_.end()) {
        if (it->second.block.dirty) {
            disk_[victimId] = it->second.block.data;
        }
        cache_.erase(it);
    }
}

void FsDriver::journalWrite(JournalOpType op, uint32_t targetId,
                             uint32_t txId, const std::string& desc) {
    JournalEntry entry;
    entry.sequenceNum = journalSeq_++;
    entry.tick = currentTick_;
    entry.opType = op;
    entry.targetId = targetId;
    entry.txId = txId;
    entry.description = desc;

    journal_.push_back(entry);
    stats_.journalEntries++;

    
    while (journal_.size() > JOURNAL_MAX_ENTRIES) {
        journal_.pop_front();
    }

    
    if (txId > 0) {
        auto txIt = activeTx_.find(txId);
        if (txIt != activeTx_.end()) {
            txIt->second.entries.push_back(entry);
        }
    }
}

void FsDriver::updateTick(uint64_t tick) {
    currentTick_ = tick;
}

} 
