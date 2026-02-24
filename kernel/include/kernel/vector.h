#pragma once

#include <stddef.h>

// Для placement new
inline void* operator new(size_t, void* p) throw() { return p; }
inline void* operator new[](size_t, void* p) throw() { return p; }
inline void  operator delete  (void*, void*) throw() { }
inline void  operator delete[](void*, void*) throw() { }

namespace re36 {

template <typename T>
class vector {
public:
    vector() : data_(nullptr), size_(0), capacity_(0) {}

    ~vector() {
        if (data_) {
            for (size_t i = 0; i < size_; ++i) {
                data_[i].~T();
            }
            delete[] reinterpret_cast<char*>(data_);
        }
    }

    void push_back(const T& value) {
        if (size_ == capacity_) {
            reserve(capacity_ == 0 ? 4 : capacity_ * 2);
        }
        new (&data_[size_]) T(value);
        size_++;
    }

    void pop_back() {
        if (size_ > 0) {
            data_[size_ - 1].~T();
            size_--;
        }
    }

    size_t size() const { return size_; }
    size_t capacity() const { return capacity_; }
    bool empty() const { return size_ == 0; }

    void reserve(size_t new_capacity) {
        if (new_capacity <= capacity_) return;

        T* new_data = reinterpret_cast<T*>(new char[new_capacity * sizeof(T)]);
        
        for (size_t i = 0; i < size_; ++i) {
            new (&new_data[i]) T(data_[i]); // Копируем 
            data_[i].~T();                  // Уничтожаем старый
        }

        if (data_) {
            delete[] reinterpret_cast<char*>(data_);
        }
        
        data_ = new_data;
        capacity_ = new_capacity;
    }

    void clear() {
        for (size_t i = 0; i < size_; ++i) {
            data_[i].~T();
        }
        size_ = 0;
    }

    T& operator[](size_t index) { return data_[index]; }
    const T& operator[](size_t index) const { return data_[index]; }

private:
    T* data_;
    size_t size_;
    size_t capacity_;
};

} // namespace re36
