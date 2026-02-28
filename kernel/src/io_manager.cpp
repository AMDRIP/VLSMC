// Не используется, не удалять
/**
 * @file io_manager.cpp
 * @brief Реализация менеджера ввода-вывода RAND Elecorner 36.
 */

#include "kernel/io_manager.h"
#include "kernel/event_bus.h"

#include <algorithm>
#include <cmath>

namespace re36 {

IOManager::IOManager(EventBus& eventBus, const KernelConfig& config)
    : eventBus_(eventBus), config_(config) {}

IOManager::~IOManager() = default;

bool IOManager::init() {
    devices_.clear();
    drivers_.clear();
    totalInterrupts_ = 0;

    // Создать стандартные устройства
    registerDevice("hdd0", DeviceType::Disk, 10);     // 10 тиков на операцию
    registerDevice("ssd0", DeviceType::Disk, 2);
    registerDevice("kbd0", DeviceType::Keyboard, 1);
    registerDevice("disp0", DeviceType::Display, 1);
    registerDevice("net0", DeviceType::Network, 5);

    return true;
}

// ---- Устройства ------------------------------------------------------------

bool IOManager::registerDevice(const std::string& name, DeviceType type, uint32_t latency) {
    if (devices_.find(name) != devices_.end()) return false;

    Device dev;
    dev.name = name;
    dev.type = type;
    dev.state = DeviceState::Idle;
    dev.latencyTicks = latency;
    dev.totalOps = 0;
    dev.totalBytesTransferred = 0;
    devices_[name] = std::move(dev);
    return true;
}

bool IOManager::removeDevice(const std::string& name) {
    auto it = devices_.find(name);
    if (it == devices_.end()) return false;
    if (!it->second.requestQueue.empty()) return false;
    devices_.erase(it);
    return true;
}

std::optional<Device> IOManager::getDevice(const std::string& name) const {
    auto it = devices_.find(name);
    return it != devices_.end() ? std::optional(it->second) : std::nullopt;
}

std::vector<Device> IOManager::getAllDevices() const {
    std::vector<Device> result;
    for (auto& [n, d] : devices_) result.push_back(d);
    return result;
}

// ---- Запросы I/O -----------------------------------------------------------

uint32_t IOManager::submitRequest(const std::string& deviceName, Pid pid,
                                   IORequestType type, const std::string& data,
                                   size_t size) {
    auto it = devices_.find(deviceName);
    if (it == devices_.end()) return 0;

    IORequest req;
    req.id = nextRequestId_++;
    req.pid = pid;
    req.deviceName = deviceName;
    req.type = type;
    req.data = data;
    req.size = size;
    req.status = IORequestStatus::Pending;
    req.submittedAt = 0; // будет заполнено при обработке
    req.remainingTicks = it->second.latencyTicks;

    uint32_t rid = req.id;
    it->second.requestQueue.push_back(std::move(req));

    Event evt(EventType::IoRequestSubmitted, 0, "io");
    evt.with("device", deviceName).with("pid", static_cast<int64_t>(pid));
    eventBus_.publish(evt);

    return rid;
}

void IOManager::tick() {
    for (auto& [name, device] : devices_) {
        if (device.state == DeviceState::Error) continue;

        // Обработка текущего запроса
        if (device.state == DeviceState::Busy && !device.requestQueue.empty()) {
            auto& req = device.requestQueue.front();
            if (req.remainingTicks > 0) {
                req.remainingTicks--;
            }
            if (req.remainingTicks == 0) {
                // Запрос завершён
                req.status = IORequestStatus::Completed;
                device.totalOps++;
                device.totalBytesTransferred += req.size;

                Pid pid = req.pid;
                device.requestQueue.erase(device.requestQueue.begin());

                // Прерывание
                generateInterrupt(name, pid);

                if (device.requestQueue.empty()) {
                    device.state = DeviceState::Idle;
                }
            }
        }

        // Начать следующий запрос
        if (device.state == DeviceState::Idle && !device.requestQueue.empty()) {
            device.state = DeviceState::Busy;
            device.requestQueue.front().status = IORequestStatus::InProgress;
        }
    }
}

std::vector<IORequest> IOManager::getDeviceQueue(const std::string& deviceName) const {
    auto it = devices_.find(deviceName);
    if (it == devices_.end()) return {};
    return it->second.requestQueue;
}

std::vector<IORequest> IOManager::getProcessRequests(Pid pid) const {
    std::vector<IORequest> result;
    for (auto& [name, dev] : devices_) {
        for (auto& req : dev.requestQueue) {
            if (req.pid == pid) result.push_back(req);
        }
    }
    return result;
}

// ---- Планирование диска ----------------------------------------------------

void IOManager::setDiskSchedulingAlgorithm(DiskSchedulingAlgorithm algo) {
    diskAlgo_ = algo;
}

DiskSchedulingAlgorithm IOManager::getDiskSchedulingAlgorithm() const {
    return diskAlgo_;
}

std::vector<uint32_t> IOManager::getDiskScheduleOrder(
        const std::vector<uint32_t>& requests, uint32_t headPos) const {
    if (requests.empty()) return {};

    switch (diskAlgo_) {
        case DiskSchedulingAlgorithm::FCFS:
            return requests;

        case DiskSchedulingAlgorithm::SSTF: {
            std::vector<uint32_t> result;
            std::vector<bool> visited(requests.size(), false);
            uint32_t current = headPos;
            for (size_t i = 0; i < requests.size(); ++i) {
                int32_t bestIdx = -1;
                uint32_t bestDist = UINT32_MAX;
                for (size_t j = 0; j < requests.size(); ++j) {
                    if (visited[j]) continue;
                    uint32_t dist = (requests[j] > current) ?
                        requests[j] - current : current - requests[j];
                    if (dist < bestDist) { bestDist = dist; bestIdx = static_cast<int32_t>(j); }
                }
                if (bestIdx >= 0) {
                    visited[static_cast<size_t>(bestIdx)] = true;
                    current = requests[static_cast<size_t>(bestIdx)];
                    result.push_back(current);
                }
            }
            return result;
        }

        case DiskSchedulingAlgorithm::SCAN: {
            std::vector<uint32_t> lower, upper;
            for (auto r : requests) {
                if (r < headPos) lower.push_back(r);
                else upper.push_back(r);
            }
            std::sort(upper.begin(), upper.end());
            std::sort(lower.begin(), lower.end(), std::greater<>());

            std::vector<uint32_t> result;
            for (auto r : upper) result.push_back(r);
            for (auto r : lower) result.push_back(r);
            return result;
        }
    }
    return requests;
}

// ---- Прерывания ------------------------------------------------------------

void IOManager::generateInterrupt(const std::string& deviceName, Pid pid) {
    totalInterrupts_++;

    Event evt(EventType::IoCompleted, 0, "io");
    evt.with("device", deviceName).with("pid", static_cast<int64_t>(pid));
    eventBus_.publish(evt);

    for (auto& cb : interruptCallbacks_) {
        cb(deviceName, pid);
    }
}

void IOManager::registerInterruptCallback(InterruptCallback cb) {
    interruptCallbacks_.push_back(std::move(cb));
}

uint64_t IOManager::getTotalInterrupts() const {
    return totalInterrupts_;
}

// ---- Очистка ---------------------------------------------------------------

void IOManager::cancelProcessRequests(Pid pid) {
    for (auto& [name, dev] : devices_) {
        dev.requestQueue.erase(
            std::remove_if(dev.requestQueue.begin(), dev.requestQueue.end(),
                [pid](const IORequest& r) { return r.pid == pid; }),
            dev.requestQueue.end()
        );
        if (dev.requestQueue.empty() && dev.state == DeviceState::Busy) {
            dev.state = DeviceState::Idle;
        }
    }
}

} // namespace re36
