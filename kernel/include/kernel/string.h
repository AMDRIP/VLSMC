#pragma once

#include <stddef.h>
#include "libc.h" // For memcpy, strlen, strcmp

// Требуется для размещения (placement new)
inline void* operator new(size_t, void* p) throw();

namespace re36 {

class string {
public:
    string() : data_(nullptr), size_(0), capacity_(0) {}

    string(const char* str) {
        size_ = strlen(str);
        capacity_ = size_ + 1; // +1 для нуль-терминатора
        data_ = new char[capacity_];
        memcpy(data_, str, size_ + 1);
    }

    string(const string& other) {
        size_ = other.size_;
        capacity_ = other.capacity_;
        if (capacity_ > 0) {
            data_ = new char[capacity_];
            memcpy(data_, other.data_, size_ + 1);
        } else {
            data_ = nullptr;
        }
    }

    ~string() {
        if (data_) {
            delete[] data_;
        }
    }

    string& operator=(const string& other) {
        if (this != &other) {
            if (data_) {
                delete[] data_;
            }
            size_ = other.size_;
            capacity_ = other.capacity_;
            if (capacity_ > 0) {
                data_ = new char[capacity_];
                memcpy(data_, other.data_, size_ + 1);
            } else {
                data_ = nullptr;
            }
        }
        return *this;
    }

    string& operator=(const char* str) {
        if (data_) {
            delete[] data_;
        }
        size_ = strlen(str);
        capacity_ = size_ + 1;
        data_ = new char[capacity_];
        memcpy(data_, str, size_ + 1);
        return *this;
    }

    string operator+(const string& other) const {
        string result;
        result.size_ = size_ + other.size_;
        result.capacity_ = result.size_ + 1;
        result.data_ = new char[result.capacity_];
        
        if (size_ > 0) memcpy(result.data_, data_, size_);
        if (other.size_ > 0) memcpy(result.data_ + size_, other.data_, other.size_ + 1);
        else result.data_[size_] = '\0';

        return result;
    }

    string& operator+=(const string& other) {
        size_t new_size = size_ + other.size_;
        if (new_size + 1 > capacity_) {
            reserve(new_size + 1);
        }
        if (other.size_ > 0) {
            memcpy(data_ + size_, other.data_, other.size_ + 1);
        }
        size_ = new_size;
        return *this;
    }

    bool operator==(const string& other) const {
        if (size_ != other.size_) return false;
        if (size_ == 0) return true;
        return strcmp(data_, other.data_) == 0;
    }

    bool operator==(const char* str) const {
        if (!data_ && !str) return true;
        if (!data_ || !str) return false;
        return strcmp(data_, str) == 0;
    }

    const char* c_str() const {
        return data_ ? data_ : "";
    }

    size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }

    void reserve(size_t new_capacity) {
        if (new_capacity <= capacity_) return;
        
        char* new_data = new char[new_capacity];
        if (data_) {
            memcpy(new_data, data_, size_ + 1);
            delete[] data_;
        } else {
            new_data[0] = '\0';
        }
        
        data_ = new_data;
        capacity_ = new_capacity;
    }

    char& operator[](size_t index) { return data_[index]; }
    const char& operator[](size_t index) const { return data_[index]; }

private:
    char* data_;
    size_t size_;
    size_t capacity_;
};

} // namespace re36
