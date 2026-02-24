/**
 * @file syscalls.cpp
 * @brief Реализация SyscallArgs и SyscallResult.
 */

#include "kernel/syscalls.h"

namespace re36 {

// ============================================================================
// SyscallArgs
// ============================================================================

void SyscallArgs::set(const std::string& key, int64_t value) {
    params[key] = value;
}

void SyscallArgs::set(const std::string& key, uint64_t value) {
    params[key] = value;
}

void SyscallArgs::set(const std::string& key, const std::string& value) {
    params[key] = value;
}

void SyscallArgs::set(const std::string& key, bool value) {
    params[key] = value;
}

int64_t SyscallArgs::getInt(const std::string& key, int64_t defaultVal) const {
    auto it = params.find(key);
    if (it != params.end()) {
        if (auto* v = std::get_if<int64_t>(&it->second)) return *v;
        if (auto* v = std::get_if<uint64_t>(&it->second)) return static_cast<int64_t>(*v);
    }
    return defaultVal;
}

uint64_t SyscallArgs::getUint(const std::string& key, uint64_t defaultVal) const {
    auto it = params.find(key);
    if (it != params.end()) {
        if (auto* v = std::get_if<uint64_t>(&it->second)) return *v;
        if (auto* v = std::get_if<int64_t>(&it->second)) return static_cast<uint64_t>(*v);
    }
    return defaultVal;
}

std::string SyscallArgs::getString(const std::string& key, const std::string& defaultVal) const {
    auto it = params.find(key);
    if (it != params.end()) {
        if (auto* v = std::get_if<std::string>(&it->second)) return *v;
    }
    return defaultVal;
}

bool SyscallArgs::getBool(const std::string& key, bool defaultVal) const {
    auto it = params.find(key);
    if (it != params.end()) {
        if (auto* v = std::get_if<bool>(&it->second)) return *v;
    }
    return defaultVal;
}

bool SyscallArgs::hasKey(const std::string& key) const {
    return params.find(key) != params.end();
}

// ============================================================================
// SyscallResult
// ============================================================================

SyscallResult SyscallResult::ok() {
    SyscallResult r;
    r.status = SyscallStatus::Ok;
    return r;
}

SyscallResult SyscallResult::ok(const std::string& key, KernelValue value) {
    SyscallResult r;
    r.status = SyscallStatus::Ok;
    r.data[key] = std::move(value);
    return r;
}

SyscallResult SyscallResult::error(SyscallStatus status, const std::string& message) {
    SyscallResult r;
    r.status = status;
    r.errorMessage = message;
    return r;
}

int64_t SyscallResult::getInt(const std::string& key, int64_t defaultVal) const {
    auto it = data.find(key);
    if (it != data.end()) {
        if (auto* v = std::get_if<int64_t>(&it->second)) return *v;
        if (auto* v = std::get_if<uint64_t>(&it->second)) return static_cast<int64_t>(*v);
    }
    return defaultVal;
}

uint64_t SyscallResult::getUint(const std::string& key, uint64_t defaultVal) const {
    auto it = data.find(key);
    if (it != data.end()) {
        if (auto* v = std::get_if<uint64_t>(&it->second)) return *v;
        if (auto* v = std::get_if<int64_t>(&it->second)) return static_cast<uint64_t>(*v);
    }
    return defaultVal;
}

std::string SyscallResult::getString(const std::string& key, const std::string& defaultVal) const {
    auto it = data.find(key);
    if (it != data.end()) {
        if (auto* v = std::get_if<std::string>(&it->second)) return *v;
    }
    return defaultVal;
}

bool SyscallResult::getBool(const std::string& key, bool defaultVal) const {
    auto it = data.find(key);
    if (it != data.end()) {
        if (auto* v = std::get_if<bool>(&it->second)) return *v;
    }
    return defaultVal;
}

} // namespace re36
