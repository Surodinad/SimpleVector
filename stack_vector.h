#pragma once

#include <array>
#include <stdexcept>

template <typename T, size_t N>
class StackVector {
public:
    explicit StackVector(size_t a_size = 0) {
        if (a_size > N) {
            throw std::invalid_argument("Expect invalid_argument for too large size");
        }
        capacity_ = N;
        size_ = a_size;
    }

    T& operator[](size_t index) {
        return vector_.at(index);
    }

    const T& operator[](size_t index) const {
        return vector_.at(index);
    }

    typename std::array<T,N>::iterator begin() {
        return vector_.begin();
    }
    typename std::array<T,N>::iterator end() {
        return vector_.begin() + size_;
    }
    typename std::array<T,N>::const_iterator begin() const {
        return vector_.cbegin();
    }
    typename std::array<T,N>::const_iterator end() const {
        return vector_.begin() + size_;
    }

    size_t Size() const {
        return size_;
    }
    size_t Capacity() const {
        return capacity_;
    }

    void PushBack(const T& value) {
        if (size_ == capacity_) {
            throw std::overflow_error("push size limited");
        }

        vector_[size_++] = value;
    }

    T PopBack() {
        if (size_ == 0) {
            throw std::underflow_error("pop size limited");
        }

        return vector_[--size_];
    }

private:
    std::array<T,N> vector_;
    size_t size_;
    size_t capacity_;
};
