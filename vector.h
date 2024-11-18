#include <alloca.h>
#include <initializer_list>
#include <memory>
#include <iterator>
#include <stdexcept>
#include <utility>


template <typename T, typename Allocator = std::allocator<T>>
class Vector {
    class Iterator;
public:
    constexpr Vector() noexcept(noexcept(Allocator())) : Vector(Allocator()) {}

    constexpr explicit Vector(const Allocator& alloc) noexcept : size_(0), capacity_(0), alloc_(alloc), data_(nullptr) {
    }

    explicit Vector(size_t count, const Allocator& alloc = Allocator()) : size_(count), capacity_(count), alloc_(alloc) {
        data_ = alloc_.allocate(count);
        for (size_t i = 0; i < size_; ++i) {
            std::allocator_traits<Allocator>::construct(alloc_, data_[i]);
        }
    }

    constexpr explicit Vector(size_t count, const T& value, const Allocator& alloc = Allocator()) : size_(count), capacity_(count), alloc_(alloc) {
        data_ = alloc_.allocate(count);
        for (size_t i = 0; i < size_; ++i) {
            std::allocator_traits<Allocator>::construct(alloc_, data_ + i, value);
        }
    }

    template <typename InputIt>
    constexpr Vector(InputIt first, InputIt last, const Allocator& alloc = Allocator()) : size_(0), capacity_(0), alloc_(alloc), data_(nullptr) {
        for (InputIt it = first; it != last; ++it) {
            PushBack(*it);
        }
    }

    constexpr Vector(const Vector& other) {
        size_ = other.size_;
        capacity_ = other.capacity_;
        alloc_ = std::allocator_traits<Allocator>::select_on_container_copy_construction(other.alloc_);

        data_ = alloc_.allocate(capacity_);
        for (size_t i = 0; i < size_; ++i) {
            std::allocator_traits<Allocator>::construct(alloc_, data_ + i, other.data_[i]);
        }
    }

    constexpr Vector(Vector&& other) noexcept {
        size_ = other.size_;
        capacity_ = other.capacity_;
        alloc_ = std::move(other.alloc_);

        data_ = other.data_;
        
        other.size_ = 0;
        other.capacity_ = 0;
        other.data_ = nullptr;
    }

    constexpr Vector(const Vector& other, const Allocator& alloc) {
        size_ = other.size_;
        capacity_ = other.capacity_;
        alloc_ = alloc;

        data_ = alloc_.allocate(capacity_);
        for (size_t i = 0; i < size_; ++i) {
            std::allocator_traits<Allocator>::construct(alloc_, data_ + i, other.data_[i]);
        }
    }

    Vector(Vector&& other, const Allocator& alloc) {
        size_ = other.size_;
        capacity_ = other.capacity_;
        alloc_ = alloc;

        data_ = alloc_.allocate(capacity_);
        for (size_t i = 0; i < size_; ++i) {
            std::allocator_traits<Allocator>::construct(alloc_, data_ + i, std::move(other.data_[i]));
        }

        other.Deallocate(other.alloc_);
        other.size_ = 0;
        other.capacity_ = 0;
        other.data_ = nullptr;
    }

    Vector(std::initializer_list<T> values, const Allocator& alloc = Allocator()) {
        size_ = values.size();
        capacity_ = values.size();
        alloc_ = alloc;

        data_ = alloc_.allocate(capacity_);
        for (auto it = values.begin(); it != values.end(); ++it) {
            std::allocator_traits<Allocator>::construct(alloc_, data_ + (it - values.begin()), *it);
        }
    }

    constexpr Vector& operator=(const Vector& other) {
        auto copy_content = [&](const Allocator& first_alloc, const Allocator& second_alloc) {
            size_ = other.size_;
            if (capacity_ < other.capacity_) {
                Deallocate(first_alloc);
                capacity_ = other.capacity_;
                second_alloc.allocate(data_, capacity_);
                for (size_t i = 0; i < size_; ++i) {
                    std::allocator_traits<Allocator>::construct(second_alloc, data_ + i, other.data_[i]);
                }
            } else {
                for (size_t i = 0; i < size_; ++i) {
                    data_[i] = other.data_[i];
                }
            }
        };

        if constexpr (std::allocator_traits<Allocator>::propagate_on_container_copy_assignment::value) {
            if (alloc_ == other.alloc_) {
                copy_content(other.alloc_, other.alloc_);
            } else {
                copy_content(alloc_, other.alloc_);
            }
            alloc_ = other.alloc_;
        } else {
            copy_content(alloc_, alloc_);
        }

        return *this;
    }

    Vector& operator=(Vector&& other) {
        auto allowed_alloc_move_content = [&]() {
            alloc_ = std::move(other.alloc_);

            size_ = other.size_;
            capacity_ = other.capacity_;
            data_ = other.data_;

            other.size_ = 0;
            other.capacity_ = 0;
            other.data_ = nullptr;
        };

        if constexpr (std::allocator_traits<Allocator>::propagate_on_container_move_assignment::value) {
            alloc_ = std::move(other.alloc_);
            allowed_alloc_move_content();
        } else {
            if (alloc_ != other.alloc_) {
                Deallocate(alloc_);
                size_ = other.size_;
                capacity_ = other.capacity_;
                data_ = alloc_.allocate(data_, capacity_);
                for (size_t i = 0; i < size_; ++i) {
                    std::allocator_traits<Allocator>::construct(alloc_, data_ + i, std::move(other.data_[i]));
                }
            } else {
                allowed_alloc_move_content();
            }
        }

        return *this;
    }

    constexpr Vector& operator=(std::initializer_list<T> values) {
        Deallocate(alloc_);
        
        size_ = values.size();
        capacity_ = size_;
        data_ = alloc_.allocate(capacity_);

        for (auto it = values.begin(); it != values.end(); ++it) {
            std::allocator_traits<Allocator>::construct(alloc_, data_ + (it - values.begin()), *it);
        }

        return *this;
    }

    Allocator& GetAllocator() {
        return alloc_;
    }

    const Allocator& GetAllocator() const {
        return alloc_;
    }

    constexpr void Assign(size_t count, const T& value) {
        Deallocate(alloc_);

        size_ = count;
        capacity_ = count;
        data_ = alloc_.allocate(capacity_);
        for (size_t i = 0; i < size_; ++i) {
            std::allocator_traits<Allocator>::construct(alloc_, data_ +  i, value);
        }
    }

    template <typename InputIt>
    constexpr void Assign(InputIt first, InputIt second) {
        Clear();
        for (InputIt it = first; it != second; ++it) {
            PushBack(it);
        }
    }

    constexpr void Assign(std::initializer_list<T> values) {
        if (data_) {
            data_.deallocate(data_, capacity_);
        }

        Deallocate(alloc_);
        
        size_ = values.size();
        capacity_ = size_;
        data_ = alloc_.allocate(capacity_);

        for (auto it = values.begin(); it != values.end(); ++it) {
            std::allocator_traits<Allocator>::construct(alloc_, data_ + (it - values.begin()), *it);
        }
    }

    constexpr Iterator begin() noexcept {
        return Iterator(data_);
    }

    constexpr const Iterator begin() const noexcept {
        return Iterator(data_);
    }

    constexpr const Iterator cbegin() const noexcept {
        return Iterator(data_);
    }

    constexpr Iterator end() noexcept {
        return Iterator(data_ + size_);
    }

    constexpr const Iterator end() const noexcept {
        return Iterator(data_ + size_);
    }

    constexpr const Iterator cend() const noexcept {
        return Iterator(data_ + size_);
    }

    // ..rbegin, crbegin, rend, crend..

    constexpr T& At(size_t idx) {
        if (idx >= size_) {
            throw std::out_of_range("Trying to access an element with index >= current size");
        }
        return data_[idx];
    }

    constexpr const T& At(size_t idx) const {
        if (idx >= size_) {
            throw std::out_of_range("Trying to access an element with index >= current size");
        }
        return data_[idx];
    }

    constexpr T& operator[](size_t idx) {
        return data_[idx];
    }

    constexpr const T& operator[](size_t idx) const {
        return data_[idx];
    }

    constexpr T& Front() {
        return *data_;
    }

    constexpr const T& Front() const{
        return *data_;
    }

    constexpr T& Back() {
        return data_[size_ - 1];
    }

    constexpr const T& Back() const {
        return data_[size_ - 1];
    }

    constexpr T* Data() noexcept {
        return data_;
    }

    constexpr const T* Data() const noexcept {
        return data_;
    }

    constexpr bool Empty() const noexcept {
        return size_ == 0;
    }

    constexpr size_t Size() const noexcept {
        return size_;
    }

    constexpr uint64_t MaxSize() const noexcept {
        return (1ull << (sizeof(void *))) / sizeof(T) - 1;
    }

    constexpr size_t Capacity() const noexcept {
        return capacity_;
    }

    constexpr void Reserve(size_t capacity) {
        if (capacity <= capacity_) {
            return;
        }

        T* new_data = alloc_.allocate(capacity);
        for (size_t i = 0; i < size_; ++i) {
            std::allocator_traits<Allocator>::construct(alloc_, new_data + i, data_[i]);
        }

        Deallocate(alloc_);
        data_ = new_data;
        capacity_ = capacity;
    }

    constexpr void ShrinkToFit() {
        if (capacity_ == size_) {
            return;
        }

        T* new_data = alloc_.allocate(size_);
        for (size_t i = 0; i < size_; ++i) {
            std::allocator_traits<Allocator>::construct(alloc_, new_data + i, data_[i]);
        }

        Deallocate(alloc_);
        data_ = new_data;
        capacity_ = size_;
    }

    constexpr void Clear() noexcept {
        size_ = 0;
    }

    constexpr void PushBack(const T& value) {
        if (size_ < capacity_) {
            std::allocator_traits<Allocator>::construct(alloc_, data_ + size_, value);
            ++size_;
            return;
        }

        if (capacity_ == 0) {
            capacity_ = 1;
            data_ = alloc_.allocate(capacity_);
            std::allocator_traits<Allocator>::construct(alloc_, data_, value);
        } else {
            T* new_data = alloc_.allocate(capacity_ * 2);
            for (size_t i = 0; i < size_; ++i) {
                std::allocator_traits<Allocator>::construct(alloc_, new_data + i, data_[i]);
            }
            std::allocator_traits<Allocator>::construct(alloc_, new_data + size_, value);

            Deallocate(alloc_);
            data_ = new_data;
            capacity_ *= 2;
        }

        ++size_;
    }

    constexpr void PushBack(T&& value) {
        if (size_ < capacity_) {
            std::allocator_traits<Allocator>::construct(alloc_, data_ + size_, std::move(value));
            ++size_;
            return;
        }

        if (capacity_ == 0) {
            capacity_ = 1;
            data_ = alloc_.allocate(capacity_);
            std::allocator_traits<Allocator>::construct(alloc_, data_, std::move(value));
        } else {
            T* new_data = alloc_.allocate(capacity_ * 2);
            for (size_t i = 0; i < size_; ++i) {
                std::allocator_traits<Allocator>::construct(alloc_, new_data + i, data_[i]);
            }
            std::allocator_traits<Allocator>::construct(alloc_, new_data + size_, std::move(value));

            Deallocate(alloc_);
            data_ = new_data;
            capacity_ *= 2;
        }

        ++size_;
    }

    constexpr void PopBack() {
        --size_;
    }

    template <typename... Args>
    void EmplaceBack(Args&&... args) {
        if (size_ < capacity_) {
            std::allocator_traits<Allocator>::construct(alloc_, data_ + size_, std::forward<Args>(args)...);
            ++size_;
            return;
        }

        if (capacity_ == 0) {
            capacity_ = 1;
            data_ = alloc_.allocate(capacity_);
            std::allocator_traits<Allocator>::construct(alloc_, data_, std::forward<Args>(args)...);
        } else {
            T* new_data = alloc_.allocate(capacity_ * 2);
            for (size_t i = 0; i < size_; ++i) {
                std::allocator_traits<Allocator>::construct(alloc_, new_data + i, data_[i]);
            }
            std::allocator_traits<Allocator>::construct(alloc_, new_data + size_, std::forward<Args>(args)...);

            Deallocate(alloc_);
            data_ = new_data;
            capacity_ *= 2;
        }

        ++size_;
    }

    void Resize(size_t count) {
        if (count <= size_) {
            size_ = count;
            return;
        }
        
        if (capacity_ < count) {
            T* new_data = alloc_.allocate(count);
            for (size_t i = 0; i < size_; ++i) {
                std::allocator_traits<Allocator>::construct(alloc_, new_data + i, data_[i]);
            }
            for (size_t i = size_; i < count; ++i) {
                std::allocator_traits<Allocator>::construct(alloc_, new_data + i);
            }
            Deallocate(alloc_);
            data_ = new_data;
            capacity_ = count;
        } else {
            for (size_t i = size_; i < count; ++i) {
                std::allocator_traits<Allocator>::construct(alloc_, data_ + i);
            }
        }

        size_ = count;
    }

    void Resize(size_t count, const T& value) {
        if (count <= size_) {
            size_ = count;
            return;
        }
        
        if (capacity_ < count) {
            T* new_data = alloc_.allocate(count);
            for (size_t i = 0; i < size_; ++i) {
                std::allocator_traits<Allocator>::construct(alloc_, new_data + i, data_[i]);
            }
            for (size_t i = size_; i < count; ++i) {
                std::allocator_traits<Allocator>::construct(alloc_, new_data + i, value);
            }
            Deallocate(alloc_);
            data_ = new_data;
            capacity_ = count;
        } else {
            for (size_t i = size_; i < count; ++i) {
                std::allocator_traits<Allocator>::construct(alloc_, data_ + i, value);
            }
        }

        size_ = count;
    }

    constexpr void Swap(Vector& other) {
        if constexpr (std::allocator_traits<Allocator>::propagate_on_container_swap::value) {
            std::swap(alloc_, other.alloc_);
        }
        std::swap(size_, other.size_);
        std::swap(capacity_, other.capacity);
        std::swap(data_, other.data_);
    }

    constexpr Iterator Insert(const Iterator pos, const T& value);

    constexpr Iterator Insert(const Iterator pos, T&& value);

    constexpr Iterator Insert(const Iterator pos, size_t count, const T& value);

    template <typename InputIt>
    constexpr Iterator Insert(const Iterator pos, InputIt first, InputIt last);

    constexpr Iterator Insert(const Iterator pos, std::initializer_list<T> values);

    template <typename... Args>
    constexpr Iterator Emplace(const Iterator pos, Args&&... args);

    constexpr Iterator Erase(const Iterator pos);

    constexpr Iterator Erase(const Iterator first, const Iterator last);

    ~Vector() {
        Deallocate(alloc_);
    }

private:
    size_t size_;
    size_t capacity_;
    Allocator alloc_;
    T* data_;

    void Deallocate(const Allocator& alloc) {
        if (!data_) {
            return;
        }
        for (size_t i = 0; i < size_; ++i) {
            std::allocator_traits<Allocator>::destroy(alloc, data_ + i);
        }
        alloc.deallocate(data_, capacity_);
    }
};


template <typename T, typename Allocator>
class Vector<T, Allocator>::Iterator {
public:

private:
    T* ptr_;
};