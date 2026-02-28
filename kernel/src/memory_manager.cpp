// Не используется, не удалять
/**
 * @file memory_manager.cpp
 * @brief Реализация менеджера памяти RAND Elecorner 36.
 */

#include "kernel/memory_manager.h"
#include "kernel/event_bus.h"

#include <algorithm>
#include <limits>

namespace re36 {

MemoryManager::MemoryManager(EventBus& eventBus, const KernelConfig& config)
    : eventBus_(eventBus),
      config_(config),
      allocAlgo_(config.allocationAlgorithm),
      replaceAlgo_(config.pageReplacement)
{
}

MemoryManager::~MemoryManager() = default;

// ---- Инициализация ---------------------------------------------------------

bool MemoryManager::init() {
    totalFrames_ = static_cast<uint32_t>(config_.totalPhysicalMemory / config_.pageSize);
    frames_.resize(totalFrames_);

    for (uint32_t i = 0; i < totalFrames_; ++i) {
        frames_[i].frameId = i;
        frames_[i].isFree = true;
        frames_[i].isKernel = false;
    }

    // Зарезервировать первые фреймы под ядро (~10% или минимум 4)
    kernelFrames_ = std::max<uint32_t>(4u, totalFrames_ / 10);
    for (uint32_t i = 0; i < kernelFrames_; ++i) {
        frames_[i].isFree = false;
        frames_[i].isKernel = true;
        frames_[i].ownerPid = KERNEL_PID;
    }

    pageFaults_ = 0;
    pageHits_ = 0;
    replacements_ = 0;
    swapOuts_ = 0;
    swapIns_ = 0;
    dirtySwapOuts_ = 0;
    swapLog_.clear();

    // Инициализация swap из конфигурации
    swapEnabled_ = config_.swapEnabled;
    if (swapEnabled_) {
        size_t swapBytes = config_.swapSize > 0
            ? config_.swapSize
            : config_.totalPhysicalMemory / 4;  // 25% RAM по умолчанию
        setSwapSize(swapBytes);
    }

    return true;
}

// ---- Выделение / освобождение ----------------------------------------------

std::optional<VirtualAddr> MemoryManager::allocate(Pid pid, size_t size) {
    uint32_t pagesNeeded = static_cast<uint32_t>((size + config_.pageSize - 1) / config_.pageSize);

    std::vector<FrameNumber> allocated;
    allocated.reserve(pagesNeeded);

    for (uint32_t i = 0; i < pagesNeeded; ++i) {
        auto frame = findFreeFrame();
        if (!frame.has_value()) {
            // Нет свободных — попробовать замещение
            FrameNumber victim = selectVictimFrame();
            evictFrame(victim);
            frame = victim;
        }

        FrameNumber f = frame.value();
        frames_[f].isFree = false;
        frames_[f].ownerPid = pid;
        frames_[f].pageNumber = static_cast<PageNumber>(i);
        frames_[f].loadedAt = 0;
        frames_[f].lastAccess = 0;
        frames_[f].dirty = false;
        frames_[f].referenced = false;
        allocated.push_back(f);
    }

    // Создать таблицу страниц для процесса
    auto& pt = pageTables_[pid];
    VirtualAddr baseAddr = static_cast<VirtualAddr>(pid * 0x10000); // Простая схема

    for (uint32_t i = 0; i < pagesNeeded; ++i) {
        PageTableEntry pte;
        pte.pageNumber = static_cast<PageNumber>(i);
        pte.frameNumber = allocated[i];
        pte.present = true;
        pte.dirty = false;
        pte.referenced = false;
        pt.push_back(pte);
    }

    // Событие
    Event evt(EventType::MemoryAllocated, 0, "memory");
    evt.with("pid", static_cast<int64_t>(pid))
       .with("size", static_cast<int64_t>(size))
       .with("pages", static_cast<int64_t>(pagesNeeded));
    eventBus_.publish(evt);

    return baseAddr;
}

void MemoryManager::deallocate(Pid pid) {
    // Освободить все фреймы процесса
    for (auto& frame : frames_) {
        if (frame.ownerPid == pid && !frame.isKernel) {
            frame.isFree = true;
            frame.ownerPid = INVALID_PID;
            frame.pageNumber = 0;
            frame.dirty = false;
            frame.referenced = false;
        }
    }

    // Освободить swap-слоты процесса
    freeSwapSlots(pid);

    pageTables_.erase(pid);

    Event evt(EventType::MemoryFreed, 0, "memory");
    evt.with("pid", static_cast<int64_t>(pid));
    eventBus_.publish(evt);
}

void MemoryManager::deallocate(Pid pid, VirtualAddr /*addr*/, size_t size) {
    uint32_t pagesToFree = static_cast<uint32_t>((size + config_.pageSize - 1) / config_.pageSize);
    uint32_t freed = 0;

    for (auto& frame : frames_) {
        if (freed >= pagesToFree) break;
        if (frame.ownerPid == pid && !frame.isKernel && !frame.isFree) {
            frame.isFree = true;
            frame.ownerPid = INVALID_PID;
            freed++;
        }
    }
}

// ---- Страничная организация ------------------------------------------------

std::optional<PhysicalAddr> MemoryManager::accessMemory(Pid pid, VirtualAddr vAddr, bool write) {
    PageNumber page = static_cast<PageNumber>(vAddr / config_.pageSize);

    auto ptIt = pageTables_.find(pid);
    if (ptIt == pageTables_.end()) {
        return std::nullopt;
    }

    auto& pt = ptIt->second;

    // Найти запись в таблице страниц
    for (auto& pte : pt) {
        if (pte.pageNumber == page) {
            if (pte.present) {
                // Page hit
                pageHits_++;
                pte.referenced = true;
                pte.lastAccess = pageFaults_ + pageHits_;
                frames_[pte.frameNumber].lastAccess = pte.lastAccess;
                frames_[pte.frameNumber].referenced = true;

                if (write) {
                    pte.dirty = true;
                    frames_[pte.frameNumber].dirty = true;
                }

                PhysicalAddr pAddr = static_cast<PhysicalAddr>(
                    pte.frameNumber * config_.pageSize + (vAddr % config_.pageSize)
                );
                return pAddr;
            } else {
                // Page fault!
                pageFaults_++;

                Event evt(EventType::PageFault, 0, "memory");
                evt.with("pid", static_cast<int64_t>(pid))
                   .with("page", static_cast<int64_t>(page));
                eventBus_.publish(evt);

                // Найти фрейм
                auto freeFrame = findFreeFrame();
                FrameNumber frame;
                if (freeFrame.has_value()) {
                    frame = freeFrame.value();
                } else {
                    frame = selectVictimFrame();
                    evictFrame(frame);  // теперь записывает в swap
                    replacements_++;

                    Event replEvt(EventType::PageReplaced, 0, "memory");
                    replEvt.with("victimFrame", static_cast<int64_t>(frame));
                    eventBus_.publish(replEvt);
                }

                // Если страница была в swap — загрузить оттуда
                if (pte.inSwap && pte.swapSlot != UINT32_MAX) {
                    readFromSwap(pte.swapSlot, pid, page, frame,
                                 pageFaults_ + pageHits_);
                    pte.inSwap = false;
                    pte.swapSlot = UINT32_MAX;
                } else {
                    loadPage(pid, page, frame, pageFaults_ + pageHits_);
                }

                pte.frameNumber = frame;
                pte.present = true;
                pte.referenced = true;
                pte.lastAccess = pageFaults_ + pageHits_;

                if (write) {
                    pte.dirty = true;
                    frames_[frame].dirty = true;
                }

                PhysicalAddr pAddr = static_cast<PhysicalAddr>(
                    frame * config_.pageSize + (vAddr % config_.pageSize)
                );
                return pAddr;
            }
        }
    }

    return std::nullopt;
}

void MemoryManager::checkPageFaults() {
    // В текущей реализации page faults обрабатываются синхронно в accessMemory
}

// ---- Настройка алгоритмов --------------------------------------------------

void MemoryManager::setAllocationAlgorithm(AllocationAlgorithm algo) {
    allocAlgo_ = algo;
}

AllocationAlgorithm MemoryManager::getAllocationAlgorithm() const {
    return allocAlgo_;
}

void MemoryManager::setPageReplacementAlgorithm(PageReplacementAlgorithm algo) {
    replaceAlgo_ = algo;
}

PageReplacementAlgorithm MemoryManager::getPageReplacementAlgorithm() const {
    return replaceAlgo_;
}

// ---- Визуализация ----------------------------------------------------------

MemoryStats MemoryManager::getStats() const {
    MemoryStats s;
    s.totalMemory = config_.totalPhysicalMemory;
    s.totalFrames = totalFrames_;

    uint32_t used = 0;
    uint32_t kernel = 0;
    for (const auto& f : frames_) {
        if (!f.isFree) {
            used++;
            if (f.isKernel) kernel++;
        }
    }

    s.usedFrames = used;
    s.freeFrames = totalFrames_ - used;
    s.usedMemory = static_cast<size_t>(used) * config_.pageSize;
    s.freeMemory = static_cast<size_t>(s.freeFrames) * config_.pageSize;
    s.kernelMemory = static_cast<size_t>(kernel) * config_.pageSize;
    s.pageFaults = pageFaults_;
    s.pageHits = pageHits_;
    s.pageReplacements = replacements_;
    s.hitRate = (pageFaults_ + pageHits_ > 0)
        ? static_cast<double>(pageHits_) / static_cast<double>(pageFaults_ + pageHits_) : 0.0;
    s.usagePercent = (totalFrames_ > 0)
        ? static_cast<double>(used) / static_cast<double>(totalFrames_) * 100.0 : 0.0;
    s.fragmentation = calculateFragmentation();
    s.swap = getSwapStats();
    return s;
}

MemoryManagerSnapshot MemoryManager::getSnapshot() const {
    MemoryManagerSnapshot snap;
    snap.totalFrames = totalFrames_;
    uint32_t used = 0;
    for (const auto& f : frames_) {
        if (!f.isFree) used++;
    }
    snap.usedFrames = used;
    snap.totalPageFaults = pageFaults_;
    snap.fragmentationPercent = calculateFragmentation() * 100.0;
    snap.swap = getSwapStats();
    return snap;
}

std::vector<MemorySegment> MemoryManager::getMemoryMap() const {
    std::vector<MemorySegment> segments;
    if (frames_.empty()) return segments;

    auto makeLabel = [](const MemoryFrame& f) -> std::string {
        if (f.isKernel) return "Ядро";
        if (f.isFree) return "Свободно";
        return "Процесс " + std::to_string(f.ownerPid);
    };

    // Объединить смежные фреймы с одинаковым статусом
    MemorySegment currentSeg;
    currentSeg.startAddr = 0;
    currentSeg.size = config_.pageSize;
    currentSeg.isFree = frames_[0].isFree;
    currentSeg.isKernel = frames_[0].isKernel;
    currentSeg.ownerPid = frames_[0].ownerPid;
    currentSeg.label = makeLabel(frames_[0]);

    for (uint32_t i = 1; i < totalFrames_; ++i) {
        const auto& f = frames_[i];
        bool same = (f.isFree == currentSeg.isFree) &&
                    (f.isKernel == currentSeg.isKernel) &&
                    (f.ownerPid == currentSeg.ownerPid);

        if (same) {
            currentSeg.size += config_.pageSize;
        } else {
            segments.push_back(currentSeg);
            currentSeg.startAddr = static_cast<PhysicalAddr>(i * config_.pageSize);
            currentSeg.size = config_.pageSize;
            currentSeg.isFree = f.isFree;
            currentSeg.isKernel = f.isKernel;
            currentSeg.ownerPid = f.ownerPid;
            currentSeg.label = makeLabel(f);
        }
    }
    segments.push_back(currentSeg);
    return segments;
}

std::vector<PageTableEntry> MemoryManager::getPageTable(Pid pid) const {
    auto it = pageTables_.find(pid);
    if (it != pageTables_.end()) {
        return it->second;
    }
    return {};
}

std::vector<MemoryFrame> MemoryManager::getFrames() const {
    return frames_;
}

size_t MemoryManager::getProcessMemoryUsage(Pid pid) const {
    size_t count = 0;
    for (const auto& f : frames_) {
        if (f.ownerPid == pid && !f.isFree) count++;
    }
    return count * config_.pageSize;
}

// ---- Внутренние методы -----------------------------------------------------

std::optional<FrameNumber> MemoryManager::findFreeFrame(size_t /*contiguousCount*/) {
    switch (allocAlgo_) {
        case AllocationAlgorithm::FirstFit:
            for (uint32_t i = kernelFrames_; i < totalFrames_; ++i) {
                if (frames_[i].isFree) return i;
            }
            break;

        case AllocationAlgorithm::BestFit: {
            // Для одного фрейма BestFit = FirstFit; разница только при contiguous
            for (uint32_t i = kernelFrames_; i < totalFrames_; ++i) {
                if (frames_[i].isFree) return i;
            }
            break;
        }

        case AllocationAlgorithm::WorstFit: {
            // Искать с конца
            for (uint32_t i = totalFrames_ - 1; i >= kernelFrames_; --i) {
                if (frames_[i].isFree) return i;
            }
            break;
        }
    }
    return std::nullopt;
}

FrameNumber MemoryManager::selectVictimFrame() {
    switch (replaceAlgo_) {
        case PageReplacementAlgorithm::FIFO:
            return selectVictimFIFO();
        case PageReplacementAlgorithm::LRU:
            return selectVictimLRU();
        case PageReplacementAlgorithm::OPT:
            return selectVictimOPT();
    }
    return selectVictimFIFO();
}

FrameNumber MemoryManager::selectVictimFIFO() {
    FrameNumber victim = kernelFrames_;
    Tick oldestLoad = std::numeric_limits<Tick>::max();

    for (uint32_t i = kernelFrames_; i < totalFrames_; ++i) {
        if (!frames_[i].isFree && !frames_[i].isKernel) {
            if (frames_[i].loadedAt < oldestLoad) {
                oldestLoad = frames_[i].loadedAt;
                victim = i;
            }
        }
    }
    return victim;
}

FrameNumber MemoryManager::selectVictimLRU() {
    FrameNumber victim = kernelFrames_;
    Tick oldestAccess = std::numeric_limits<Tick>::max();

    for (uint32_t i = kernelFrames_; i < totalFrames_; ++i) {
        if (!frames_[i].isFree && !frames_[i].isKernel) {
            if (frames_[i].lastAccess < oldestAccess) {
                oldestAccess = frames_[i].lastAccess;
                victim = i;
            }
        }
    }
    return victim;
}

FrameNumber MemoryManager::selectVictimOPT() {
    // В симуляторе OPT = LRU (предсказывать нечего)
    return selectVictimLRU();
}

void MemoryManager::loadPage(Pid pid, PageNumber page, FrameNumber frame, Tick currentTick) {
    frames_[frame].isFree = false;
    frames_[frame].isKernel = false;
    frames_[frame].ownerPid = pid;
    frames_[frame].pageNumber = page;
    frames_[frame].loadedAt = currentTick;
    frames_[frame].lastAccess = currentTick;
    frames_[frame].dirty = false;
    frames_[frame].referenced = true;
}

void MemoryManager::evictFrame(FrameNumber frame) {
    auto& f = frames_[frame];
    Pid oldOwner = f.ownerPid;
    PageNumber oldPage = f.pageNumber;
    bool wasDirty = f.dirty;

    // Если swap включён — сохранить страницу в swap
    if (swapEnabled_ && oldOwner != INVALID_PID && oldOwner != KERNEL_PID) {
        auto slot = findFreeSwapSlot();
        if (slot.has_value()) {
            writeToSwap(oldOwner, oldPage, frame, slot.value(),
                        pageFaults_ + pageHits_);

            // Обновить PTE старого владельца — отметить как inSwap
            auto ptIt = pageTables_.find(oldOwner);
            if (ptIt != pageTables_.end()) {
                for (auto& pte : ptIt->second) {
                    if (pte.pageNumber == oldPage && pte.frameNumber == frame) {
                        pte.present = false;
                        pte.inSwap = true;
                        pte.swapSlot = slot.value();
                        break;
                    }
                }
            }
        } else {
            // Swap полон — просто сбросить (потеря данных)
            auto ptIt = pageTables_.find(oldOwner);
            if (ptIt != pageTables_.end()) {
                for (auto& pte : ptIt->second) {
                    if (pte.pageNumber == oldPage && pte.frameNumber == frame) {
                        pte.present = false;
                        pte.inSwap = false;
                        break;
                    }
                }
            }
        }
    } else {
        // Swap выключен — просто сбросить PTE
        auto ptIt = pageTables_.find(oldOwner);
        if (ptIt != pageTables_.end()) {
            for (auto& pte : ptIt->second) {
                if (pte.pageNumber == oldPage && pte.frameNumber == frame) {
                    pte.present = false;
                    break;
                }
            }
        }
    }

    f.isFree = true;
    f.ownerPid = INVALID_PID;
    f.dirty = false;
    f.referenced = false;
    (void)wasDirty;
}

double MemoryManager::calculateFragmentation() const {
    // Внешняя фрагментация = 1 - (макс. свободный блок / всего свободно)
    uint32_t totalFree = 0;
    uint32_t maxBlock = 0;
    uint32_t currentBlock = 0;

    for (uint32_t i = kernelFrames_; i < totalFrames_; ++i) {
        if (frames_[i].isFree) {
            totalFree++;
            currentBlock++;
            if (currentBlock > maxBlock) maxBlock = currentBlock;
        } else {
            currentBlock = 0;
        }
    }

    if (totalFree == 0) return 0.0;
    return 1.0 - static_cast<double>(maxBlock) / static_cast<double>(totalFree);
}

// ---- Swap-методы ----------------------------------------------------------------

void MemoryManager::setSwapEnabled(bool enabled) {
    swapEnabled_ = enabled;
    if (enabled && swapSlots_.empty()) {
        setSwapSize(config_.totalPhysicalMemory / 4);
    }
}

bool MemoryManager::isSwapEnabled() const {
    return swapEnabled_;
}

void MemoryManager::setSwapSize(size_t bytes) {
    totalSwapSlots_ = static_cast<uint32_t>(bytes / config_.pageSize);
    if (totalSwapSlots_ < 4) totalSwapSlots_ = 4;

    swapSlots_.resize(totalSwapSlots_);
    for (uint32_t i = 0; i < totalSwapSlots_; ++i) {
        swapSlots_[i].slotId = i;
        swapSlots_[i].isFree = true;
        swapSlots_[i].ownerPid = INVALID_PID;
        swapSlots_[i].pageNumber = 0;
        swapSlots_[i].dirty = false;
        swapSlots_[i].swappedAt = 0;
    }
}

SwapStats MemoryManager::getSwapStats() const {
    SwapStats s;
    s.totalSlots = totalSwapSlots_;
    uint32_t used = 0;
    for (const auto& slot : swapSlots_) {
        if (!slot.isFree) used++;
    }
    s.usedSlots = used;
    s.freeSlots = totalSwapSlots_ - used;
    s.usagePercent = (totalSwapSlots_ > 0)
        ? (100.0 * used / totalSwapSlots_) : 0.0;
    s.totalSwapOuts = swapOuts_;
    s.totalSwapIns = swapIns_;
    s.dirtySwapOuts = dirtySwapOuts_;
    s.totalSwapBytes = static_cast<size_t>(totalSwapSlots_) * config_.pageSize;
    s.usedSwapBytes = static_cast<size_t>(used) * config_.pageSize;
    return s;
}

std::vector<SwapSlot> MemoryManager::getSwapSlots() const {
    return swapSlots_;
}

std::vector<SwapLogEntry> MemoryManager::getSwapLog(size_t count) const {
    if (count == 0 || count >= swapLog_.size()) {
        return {swapLog_.begin(), swapLog_.end()};
    }
    return {swapLog_.end() - static_cast<ptrdiff_t>(count), swapLog_.end()};
}

bool MemoryManager::swapOutPage(Pid pid, PageNumber page) {
    if (!swapEnabled_) return false;

    auto ptIt = pageTables_.find(pid);
    if (ptIt == pageTables_.end()) return false;

    for (auto& pte : ptIt->second) {
        if (pte.pageNumber == page && pte.present) {
            auto slot = findFreeSwapSlot();
            if (!slot.has_value()) return false;

            FrameNumber frame = pte.frameNumber;
            writeToSwap(pid, page, frame, slot.value(), pageFaults_ + pageHits_);

            pte.present = false;
            pte.inSwap = true;
            pte.swapSlot = slot.value();

            // Освободить фрейм
            frames_[frame].isFree = true;
            frames_[frame].ownerPid = INVALID_PID;
            frames_[frame].dirty = false;
            frames_[frame].referenced = false;

            return true;
        }
    }
    return false;
}

bool MemoryManager::swapInPage(Pid pid, PageNumber page) {
    auto ptIt = pageTables_.find(pid);
    if (ptIt == pageTables_.end()) return false;

    for (auto& pte : ptIt->second) {
        if (pte.pageNumber == page && pte.inSwap && !pte.present) {
            auto freeFrame = findFreeFrame();
            FrameNumber frame;
            if (freeFrame.has_value()) {
                frame = freeFrame.value();
            } else {
                frame = selectVictimFrame();
                evictFrame(frame);
                replacements_++;
            }

            readFromSwap(pte.swapSlot, pid, page, frame,
                         pageFaults_ + pageHits_);

            pte.frameNumber = frame;
            pte.present = true;
            pte.inSwap = false;
            pte.swapSlot = UINT32_MAX;
            pte.referenced = true;
            pte.lastAccess = pageFaults_ + pageHits_;

            return true;
        }
    }
    return false;
}

// ---- Swap внутренние -------------------------------------------------------

std::optional<uint32_t> MemoryManager::findFreeSwapSlot() {
    for (uint32_t i = 0; i < totalSwapSlots_; ++i) {
        if (swapSlots_[i].isFree) return i;
    }
    return std::nullopt;
}

void MemoryManager::writeToSwap(Pid pid, PageNumber page, FrameNumber frame,
                                 uint32_t slot, Tick tick) {
    bool wasDirty = frames_[frame].dirty;

    swapSlots_[slot].isFree = false;
    swapSlots_[slot].ownerPid = pid;
    swapSlots_[slot].pageNumber = page;
    swapSlots_[slot].dirty = wasDirty;
    swapSlots_[slot].swappedAt = tick;

    swapOuts_++;
    if (wasDirty) dirtySwapOuts_++;

    // Журнал
    SwapLogEntry entry;
    entry.tick = tick;
    entry.type = SwapEventType::SwapOut;
    entry.pid = pid;
    entry.page = page;
    entry.swapSlot = slot;
    entry.frame = frame;
    entry.wasDirty = wasDirty;
    swapLog_.push_back(entry);
    if (swapLog_.size() > maxSwapLog_) {
        swapLog_.pop_front();
    }

    // Событие
    Event evt(EventType::PageSwappedOut, 0, "memory");
    evt.with("pid", static_cast<int64_t>(pid))
       .with("page", static_cast<int64_t>(page))
       .with("slot", static_cast<int64_t>(slot))
       .with("dirty", wasDirty ? 1LL : 0LL);
    eventBus_.publish(evt);
}

void MemoryManager::readFromSwap(uint32_t slot, Pid pid, PageNumber page,
                                  FrameNumber frame, Tick tick) {
    // Загрузить в фрейм
    loadPage(pid, page, frame, tick);

    // Если страница была dirty при выгрузке — восстановить dirty
    if (swapSlots_[slot].dirty) {
        frames_[frame].dirty = true;
    }

    // Освободить слот
    swapSlots_[slot].isFree = true;
    swapSlots_[slot].ownerPid = INVALID_PID;
    swapSlots_[slot].pageNumber = 0;
    swapSlots_[slot].dirty = false;

    swapIns_++;

    // Журнал
    SwapLogEntry entry;
    entry.tick = tick;
    entry.type = SwapEventType::SwapIn;
    entry.pid = pid;
    entry.page = page;
    entry.swapSlot = slot;
    entry.frame = frame;
    entry.wasDirty = false;
    swapLog_.push_back(entry);
    if (swapLog_.size() > maxSwapLog_) {
        swapLog_.pop_front();
    }

    // Событие
    Event evt(EventType::PageSwappedIn, 0, "memory");
    evt.with("pid", static_cast<int64_t>(pid))
       .with("page", static_cast<int64_t>(page))
       .with("slot", static_cast<int64_t>(slot));
    eventBus_.publish(evt);
}

void MemoryManager::freeSwapSlots(Pid pid) {
    for (auto& slot : swapSlots_) {
        if (slot.ownerPid == pid) {
            slot.isFree = true;
            slot.ownerPid = INVALID_PID;
            slot.pageNumber = 0;
            slot.dirty = false;
        }
    }
}

} // namespace re36
