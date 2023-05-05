#pragma once

#include <cassert>
#include <initializer_list>
#include <iterator>
#include <algorithm>
#include <stdexcept>

#include "array_ptr.h"

class ReserveProxyObj {
public:
    explicit ReserveProxyObj(size_t capacity_to_reserve) :
            capacity_to_reserve_(capacity_to_reserve)
    {}

    size_t Get() const {
        return capacity_to_reserve_;
    }

private:
    size_t capacity_to_reserve_;
};

ReserveProxyObj Reserve(size_t capacity_to_reserve) {
    return ReserveProxyObj(capacity_to_reserve);
}

template <typename Type>
class SimpleVector {
public:
    using Iterator = Type*;
    using ConstIterator = const Type*;

    SimpleVector() noexcept = default;

    // Создаёт вектор из size элементов, инициализированных значением по умолчанию
    explicit SimpleVector(size_t size) :
        items_(size),
        size_(size),
        capacity_(size)
    {
        std::generate(begin(), end(), [] { return Type(); });
    }

    // Создаёт вектор из size элементов, инициализированных значением value
    SimpleVector(size_t size, const Type& value) :
        items_(size),
        size_(size),
        capacity_(size)
    {
        std::fill(begin(), end(), value);
    }

    // Создаёт вектор из std::initializer_list
    SimpleVector(std::initializer_list<Type> init) :
        items_(init.size()),
        size_(init.size()),
        capacity_(init.size())
    {
        std::copy(init.begin(), init.end(), begin());
    }

    SimpleVector(ReserveProxyObj reserve_proxy_obj) :
        items_(reserve_proxy_obj.Get()),
        capacity_(reserve_proxy_obj.Get())
    {
    }

    SimpleVector(const SimpleVector& other) {
        assert(size_ == 0 && items_.Get() == nullptr);

        ArrayPtr<Type> tmp_array(other.capacity_);
        std::copy(other.begin(), other.end(), Iterator(tmp_array.Get()));
        items_.swap(tmp_array);
        size_ = other.GetSize();
        capacity_ = other.GetCapacity();
    }

    SimpleVector(SimpleVector&& other) noexcept {
        swap(other);
    }

    SimpleVector& operator=(const SimpleVector& rhs) {
        if (this == &rhs) {
            return *this;
        }
        SimpleVector tmp(rhs);
        swap(tmp);
        return *this;
    }

    SimpleVector& operator=(SimpleVector&& rhs) noexcept {
        if (this == &rhs) {
            return *this;
        }
        SimpleVector tmp(std::move(rhs));
        swap(tmp);
        return *this;
    }

    // Возвращает количество элементов в массиве
    size_t GetSize() const noexcept {
        return size_;
    }

    // Возвращает вместимость массива
    size_t GetCapacity() const noexcept {
        return capacity_;
    }

    // Сообщает, пустой ли массив
    bool IsEmpty() const noexcept {
        return size_ == 0;
    }

    // Возвращает ссылку на элемент с индексом index
    Type& operator[](size_t index) noexcept {
        assert(index < size_);
        return items_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    const Type& operator[](size_t index) const noexcept {
        assert(index < size_);
        return items_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    Type& At(size_t index) {
        if (index >= size_) {
            throw std::out_of_range("out of range");
        }
        return items_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    const Type& At(size_t index) const {
        if (index >= size_) {
            throw std::out_of_range("out of range");
        }
        return items_[index];
    }

    // Обнуляет размер массива, не изменяя его вместимость
    void Clear() noexcept {
        size_ = 0;
    }

    // Изменяет размер массива.
    // При увеличении размера новые элементы получают значение по умолчанию для типа Type
    void Resize(size_t new_size) {
        if (new_size <= size_) {
            size_ = new_size;
        } else if (new_size <= capacity_) {
            std::generate(end(), items_.Get() + new_size, [] { return Type(); });
            size_ = new_size;
        } else {
            Reserve(new_size);
            std::generate(end(), items_.Get() + new_size, [] { return Type(); });
            size_ = new_size;
        }
    }

    void Reserve(size_t new_capacity) {
        if (new_capacity > capacity_) {
            SimpleVector<Type> new_vector(new_capacity);
            std::move(begin(), end(), new_vector.begin());
            items_.swap(new_vector.items_);
            capacity_ = new_capacity;
        }
    }

    // Добавляет элемент в конец вектора
    // При нехватке места увеличивает вдвое вместимость вектора
    void PushBack(const Type& item) {
        if (IsEmpty()) {
            if (!capacity_) {
                Reserve(10);
            }
            items_[0] = item;
            ++size_;
            return;
        }

        if (size_ < capacity_) {
            items_[size_] = item;
            ++size_;
        } else {
            ArrayPtr<Type> new_vector(capacity_ * 2);
            std::copy(items_.Get(), items_.Get() + size_, new_vector.Get());
            new_vector[size_] = item;
            items_.swap(new_vector);
            ++size_;
            capacity_ = capacity_ * 2;
        }
    }

    void PushBack(Type&& item) {
        if (IsEmpty()) {
            if (!capacity_) {
                Reserve(10);
            }
            items_[0] = std::move(item);
            ++size_;
            return;
        }

        if (size_ < capacity_) {
            items_[size_] = std::move(item);
            ++size_;
        } else {
            ArrayPtr<Type> new_vector(capacity_ * 2);
            std::move(items_.Get(), items_.Get() + size_, new_vector.Get());
            new_vector[size_] = std::move(item);
            items_.swap(new_vector);
            ++size_;
            capacity_ = capacity_ * 2;
        }
    }

    // Вставляет значение value в позицию pos.
    // Возвращает итератор на вставленное значение
    // Если перед вставкой значения вектор был заполнен полностью,
    // вместимость вектора должна увеличиться вдвое, а для вектора вместимостью 0 стать равной 1
    Iterator Insert(ConstIterator pos, const Type& value) {
        assert(Iterator(pos) >= begin() && Iterator(pos) <= end());
        if (pos == end()) {
            PushBack(value);
            return end() - 1;
        }
        if (size_ < capacity_) {
            std::copy_backward(Iterator(pos), end(), end() + 1);
            *(Iterator(pos)) = value;
            ++size_;
            return Iterator(pos);
        } else {
            SimpleVector<Type> swap_ptr((2 * capacity_));
            std::copy(begin(), Iterator(pos), swap_ptr.begin());
            std::copy(Iterator(pos), end(), swap_ptr.begin() + (Iterator(pos) - begin() + 1));
            auto return_it = swap_ptr.begin() + (pos - begin());
            *return_it = value;
            capacity_ = 2 * capacity_;
            ++size_;
            items_.swap(swap_ptr.items_);
            return return_it;
        }
    }

    Iterator Insert(ConstIterator pos, Type&& value) {
        assert(Iterator(pos) >= begin() && Iterator(pos) <= end());
        if (pos == end()) {
            PushBack(std::move(value));
            return end() - 1;
        }
        if (size_ < capacity_) {
            for (auto p = end(); p != Iterator(pos); --p) {
                *p = std::move(*(p-1));
            }
            *(Iterator(pos)) = std::move(value);
            ++size_;
            return Iterator(pos);
        } else {
            SimpleVector<Type> swap_ptr((2 * capacity_));
            std::move(begin(), Iterator(pos), swap_ptr.begin());
            std::move(Iterator(pos), end(), swap_ptr.begin() + (Iterator(pos) - begin() + 1));
            auto return_it = swap_ptr.begin() + (pos - begin());
            *return_it = std::move(value);
            capacity_ = 2 * capacity_;
            ++size_;
            items_.swap(swap_ptr.items_);
            return return_it;
        }
    }

    // "Удаляет" последний элемент вектора. Вектор не должен быть пустым
    void PopBack() noexcept {
        assert(!IsEmpty());
        --size_;
    }

    // Удаляет элемент вектора в указанной позиции
    Iterator Erase(ConstIterator pos) {
        assert(!IsEmpty());
        assert(Iterator(pos) >= begin() && Iterator(pos) < end());
        std::move(std::next(Iterator(pos)), end(), Iterator(pos));
        --size_;
        return Iterator(pos);
    }

    // Обменивает значение с другим вектором
    void swap(SimpleVector& other) noexcept {
        items_.swap(other.items_);
        std::swap(size_, other.size_);
        std::swap(capacity_, other.capacity_);
    }

    void swap(SimpleVector&& other) noexcept {
        items_.swap(other.items_);
        std::swap(size_, other.size_);
        std::swap(capacity_, other.capacity_);
    }

    // Возвращает итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    Iterator begin() noexcept {
        return items_.Get();
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    Iterator end() noexcept {
        return items_.Get() + size_;
    }

    // Возвращает константный итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator begin() const noexcept {
        return items_.Get();
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator end() const noexcept {
        return items_.Get() + size_;
    }

    // Возвращает константный итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator cbegin() const noexcept {
        return items_.Get();
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator cend() const noexcept {
        return items_.Get() + size_;
    }

private:
    ArrayPtr<Type> items_;
    size_t size_ = 0;
    size_t capacity_ = 0;
};

template <typename Type>
inline bool operator==(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return (lhs >= rhs) && (lhs <= rhs);
}

template <typename Type>
inline bool operator!=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs == rhs);
}

template <typename Type>
inline bool operator<(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename Type>
inline bool operator<=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs > rhs);
}

template <typename Type>
inline bool operator>(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return rhs < lhs;
}

template <typename Type>
inline bool operator>=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs < rhs);
}
