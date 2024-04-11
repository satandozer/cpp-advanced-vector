#pragma once
#include <cassert>
#include <cstdlib>
#include <new>
#include <utility>
#include <iostream>
#include <exception>
#include <memory>
#include <algorithm>

template <typename T>
class RawMemory {
public:

    // ---------- Конструкторы ----------

    RawMemory() = default;

    explicit RawMemory(size_t capacity)
        : buffer_(Allocate(capacity))
        , capacity_(capacity) {
    }

    RawMemory(const RawMemory&) = delete;

    RawMemory(RawMemory&& other) noexcept {
        buffer_ = std::move(other.buffer_);
        capacity_ = std::move(other.capacity_);
    }

    // ---------- Деструктор ----------

    ~RawMemory() {
        Deallocate(buffer_);
    }

    // ---------- Операторы ----------

    T* operator+(size_t offset) noexcept {
        assert(offset <= capacity_);
        return buffer_ + offset;
    }

    const T* operator+(size_t offset) const noexcept {
        return const_cast<RawMemory&>(*this) + offset;
    }

    const T& operator[](size_t index) const noexcept {
        return const_cast<RawMemory&>(*this)[index];
    }

    T& operator[](size_t index) noexcept {
        assert(index < capacity_);
        return buffer_[index];
    }

    RawMemory& operator=(const RawMemory& rhs) = delete;

    RawMemory& operator=(RawMemory&& rhs) noexcept {
        buffer_ = std::move(rhs.buffer_);
        capacity_ = std::move(rhs.capacity_);
        return *this;
    }

    // ---------- Методы ----------

    void Swap(RawMemory& other) noexcept {
        std::swap(buffer_, other.buffer_);
        std::swap(capacity_, other.capacity_);
    }

    const T* GetAddress() const noexcept {
        return buffer_;
    }

    T* GetAddress() noexcept {
        return buffer_;
    }

    size_t Capacity() const {
        return capacity_;
    }

private:
    // Выделяет сырую память под n элементов и возвращает указатель на неё
    static T* Allocate(size_t n) {
        return n != 0 ? static_cast<T*>(operator new(n * sizeof(T))) : nullptr;
    }

    // Освобождает сырую память, выделенную ранее по адресу buf при помощи Allocate
    static void Deallocate(T* buf) noexcept {
        operator delete(buf);
    }

    T* buffer_ = nullptr;
    size_t capacity_ = 0;
};

template <typename T>
class Vector {
public:

    //---------- Конструкторы ----------

    Vector() = default;

    // Резревирует заданный размер, инициализируя значениями по умолчанию
    explicit Vector(size_t size)
        : data_(size)
        , size_(size) {
            std::uninitialized_value_construct_n(data_.GetAddress(),size);
        }

    Vector(const Vector& other)
        : data_(other.size_)
        , size_(other.size_){
        std::uninitialized_copy_n(other.data_.GetAddress(),size_,data_.GetAddress());
    }

    Vector(Vector&& other) noexcept
        : data_()
        , size_(std::move(other.size_))
    {
        data_.Swap(other.data_);
        other.size_ = 0;
    }

    //---------- Деструктор ----------

    ~Vector() {
        std::destroy_n(data_.GetAddress(),size_);
    }

    //---------- Перегрузка операторов ----------

    const T& operator[](size_t index) const noexcept {
        return const_cast<Vector&>(*this)[index];
    }

    T& operator[](size_t index) noexcept {
        assert(index < size_);
        return data_.GetAddress()[index];
    }

    Vector& operator=(const Vector& rhs){
        if (this != &rhs) {
            if (rhs.size_ > data_.Capacity()) {
                Vector rhs_copy(rhs);
                Swap(rhs_copy);
            } else {
                if (rhs.size_ < size_){
                    for(size_t i = 0; i != rhs.size_;++i){
                        data_.GetAddress()[i] = rhs[i];
                    }
                    std::destroy_n(data_.GetAddress()+rhs.size_-1,size_-rhs.size_);
                    size_ = rhs.size_;
                } else {
                    for(size_t i = 0; i != size_;++i){
                        data_.GetAddress()[i] = rhs[i];
                    }
                    std::uninitialized_copy_n(rhs.data_.GetAddress()+size_,rhs.size_-size_,data_.GetAddress()+size_);
                    size_ = rhs.size_;
                }
            }
        }
        return *this;
    }

    Vector& operator=(Vector&& rhs) noexcept{
        Swap(rhs);
        return *this;
    }

    // ---------- Итераторы ----------

    using iterator = T*;
    using const_iterator = const T*;
    
    iterator begin() noexcept {
        return data_.GetAddress();
    }

    iterator end() noexcept {
        return data_+size_;
    }

    const_iterator begin() const noexcept {
        return data_.GetAddress();
    }

    const_iterator end() const noexcept {
        return data_+size_;
    }
    
    const_iterator cbegin() const noexcept {
        return begin();
    }
    
    const_iterator cend() const noexcept {
        return end();
    }
    
    //---------- Методы ----------

    size_t Size() const noexcept {
        return size_;
    }

    size_t Capacity() const noexcept {
        return data_.Capacity();
    }

    void Reserve(size_t new_capacity) {
        if (new_capacity <= Capacity()) {
            return;
        }
        RawMemory<T> new_data(new_capacity);
        if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
            std::uninitialized_move_n(begin(), size_, new_data.GetAddress());
        } else {
            std::uninitialized_copy_n(begin(), size_, new_data.GetAddress());
        }
        std::destroy_n(begin(), size_);
        data_.Swap(new_data);
    }

    void Swap(Vector& other) noexcept {
        data_.Swap(other.data_);
        size_t size_b = other.size_;
        other.size_ = size_;
        size_ = size_b;
    }

    void Resize(size_t new_size){
        if (new_size < size_){
            std::destroy_n(data_.GetAddress()+new_size-1,size_-new_size);
        } else {
            Reserve(new_size);
            std::uninitialized_value_construct_n(data_.GetAddress()+size_,new_size-size_);
        }
        size_ = new_size;
    }

    void PushBack(const T& value){
        EmplaceBack(value);
    }

    void PushBack(T&& value) {
        EmplaceBack(std::move(value));
    }

    void PopBack() /* noexcept */ {
        std::destroy_at(end()-1);
        --size_;
    }
    
    template <typename... Args>
    T& EmplaceBack(Args&&... args){
        if (size_ == Capacity()){
            RawMemory<T> new_data(size_ == 0 ? 1 : size_ * 2);
            new (&new_data[size_]) T(std::forward<Args>(args)...);
            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                std::uninitialized_move_n(begin(), size_, new_data.GetAddress());
            } else {
                std::uninitialized_copy_n(begin(), size_, new_data.GetAddress());
            }
            std::destroy_n(begin(), size_);
            data_.Swap(new_data);
        } else {
            new (&data_[size_]) T(std::forward<Args>(args)...);
        }
        ++size_;
        return data_[size_-1];
    }

    template <typename... Args>
    iterator Emplace(const_iterator pos, Args&&... args) {
        auto distance = std::distance(cbegin(),pos);
        iterator position;
        if (pos == cend()){
            EmplaceBack(std::forward<Args>(args)...);
            position = end()-1;
        } else if (size_ == Capacity()){
            RawMemory<T> new_data(size_ == 0 ? 1 : size_ * 2);
            position = new (&new_data[static_cast<size_t>(distance)]) T(std::forward<Args>(args)...);
            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                std::uninitialized_move_n(begin(), distance, new_data.GetAddress());
                std::uninitialized_move_n(begin()+distance, size_-static_cast<size_t>(distance), 
                    new_data+static_cast<size_t>(distance)+1);
            } else {
                std::uninitialized_copy_n(begin(), distance, new_data.GetAddress());
                std::uninitialized_copy_n(begin()+distance, size_-static_cast<size_t>(distance), 
                    new_data+static_cast<size_t>(distance)+1);
            }
            std::destroy_n(data_.GetAddress(), size_);
            data_.Swap(new_data);
            ++size_;
        } else {
            position = std::next(begin(),distance);
            T new_elem(std::forward<Args>(args)...);
            new (&data_[size_]) T(std::move(*(end()-1)));
            std::move_backward(position,end()-1,end());
            data_[static_cast<size_t>(distance)] = std::move(new_elem);
            ++size_;
        }
        return position;
    }

    iterator Erase(const_iterator pos) /*noexcept(std::is_nothrow_move_assignable_v<T>)*/ {
        auto distance = std::distance(cbegin(),pos);
        iterator position = begin() + distance;
        std::move(position+1,end(),position);
        std::destroy_n(end()-1,1);
        --size_;
        return position;
    }

    iterator Insert(const_iterator pos, const T& value) {
        return Emplace(pos,value);
    }

    iterator Insert(const_iterator pos, T&& value){
        return Emplace(pos,std::move(value));
    }


private:

    RawMemory<T> data_;
    size_t size_ = 0;

};