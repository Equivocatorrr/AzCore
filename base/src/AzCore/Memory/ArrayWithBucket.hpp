/*
    File: ArrayWithBucket.hpp
    Author: Philip Haynes
    A variation on ArrayWithBucket that has some static storage space for small Arrays.
*/
#ifndef AZCORE_ARRAYWITHBUCKET_HPP
#define AZCORE_ARRAYWITHBUCKET_HPP

#include "../basictypes.hpp"
#include "Array.hpp"
#include <stdexcept> // std::out_of_range
#include <initializer_list>
#include <type_traits> // std::is_trivially_copyable
#include <cstring>     // memcpy

namespace AzCore {

template <typename T, i32 noAllocCount, i32 allocTail=0>
struct ArrayWithBucket {
    static_assert(noAllocCount > 0);
    T *data;
    i32 allocated;
    i32 size;
    T noAllocData[noAllocCount];

    inline void force_inline
    _SetTerminator() {
        if constexpr (allocTail > 1) {
            for (i32 i = size; i < size + allocTail; i++) {
                data[i] = StringTerminators<T>::value;
            }
        } else if constexpr (allocTail == 1) {
            data[size] = StringTerminators<T>::value;
        }
    }
    inline void force_inline
    _Initialize(i32 newSize) {
        size = newSize;
        if (newSize > noAllocCount-allocTail) {
            allocated = newSize;
            data = new T[allocated + allocTail];
        } else {
            allocated = 0;
            data = noAllocData;
        }
    }
    inline void force_inline
    _Deinitialize() {
        if (allocated != 0) {
            delete[] data;
        }
    }

    inline void force_inline
    _Copy(const ArrayWithBucket &other) {
        if constexpr (std::is_trivially_copyable<T>::value) {
            memcpy((void *)data, (void *)other.data, sizeof(T) * size);
        } else {
            for (i32 i = 0; i < size; i++) {
                data[i] = other.data[i];
            }
        }
    }

    template<i32 otherAllocTail>
    inline void force_inline
    _Copy(const Array<T, otherAllocTail> &other) {
        if constexpr (std::is_trivially_copyable<T>::value) {
            memcpy((void *)data, (void *)other.data, sizeof(T) * size);
        } else {
            for (i32 i = 0; i < size; i++) {
                data[i] = other.data[i];
            }
        }
    }

    inline void force_inline
    _Copy(const std::initializer_list<T> &init) {
        i32 i = 0;
        for (const T &val : init) {
            data[i++] = val;
        }
    }
    // Let go of allocations without deleting them.
    inline void force_inline
    _Drop() {
        data = noAllocData;
        size = 0;
        allocated = 0;
    }
    // Take the allocations and/or values from another ArrayWithBucket.
    inline void force_inline
    _Acquire(ArrayWithBucket &&other) {
        allocated = other.allocated;
        size = other.size;
        if (allocated) {
            data = other.data;
        } else {
            data = noAllocData;
            _Copy(other);
            _SetTerminator();
        }
    }
    template<i32 otherAllocTail>
    inline void force_inline
    _Acquire(Array<T, otherAllocTail> &&other) {
        allocated = other.allocated;
        size = other.size;
        if (allocated) {
            data = other.data;
        } else {
            data = noAllocData;
            _Copy(other);
            _SetTerminator();
        }
    }
    void Clear() {
        _Deinitialize();
        _Drop();
        _SetTerminator();
    }

    ArrayWithBucket() {
        _Initialize(0);
        _SetTerminator();
    }
    ArrayWithBucket(i32 newSize) {
        _Initialize(newSize);
        _SetTerminator();
    }
    ArrayWithBucket(i32 newSize, const T &value) {
        _Initialize(newSize);
        for (i32 i = 0; i < size; i++) {
            data[i] = value;
        }
        _SetTerminator();
    }
    inline force_inline
    ArrayWithBucket(u32 newSize) : ArrayWithBucket((i32)newSize) {}
    inline force_inline
    ArrayWithBucket(u32 newSize, const T &value) : ArrayWithBucket((i32)newSize, value) {}

    ArrayWithBucket(const std::initializer_list<T> &init) {
        _Initialize(init.size());
        _Copy(init);
        _SetTerminator();
    }

    ArrayWithBucket(const ArrayWithBucket &other) {
        _Initialize(other.size);
        _Copy(other);
        _SetTerminator();
    }

    ArrayWithBucket(ArrayWithBucket &&other) noexcept {
        _Acquire(std::move(other));
        other._Drop();
    }

    template<i32 otherAllocTail>
    ArrayWithBucket(const Array<T, otherAllocTail> &other) {
        _Initialize(other.size);
        _Copy(other);
        _SetTerminator();
    }

    template<i32 otherAllocTail>
    ArrayWithBucket(Array<T, otherAllocTail> &&other) noexcept {
        _Acquire(std::move(other));
        other._Drop();
    }

    ArrayWithBucket(const T *string) {
        _Initialize(StringLength(string));
        if constexpr (std::is_trivially_copyable<T>::value) {
            memcpy((void *)data, (void *)string, sizeof(T) * size);
        } else {
            for (i32 i = 0; i < size; i++) {
                data[i] = string[i];
            }
        }
        _SetTerminator();
    }

    ArrayWithBucket(const Range<T> &range) {
        _Initialize(range.size);
        if (range.index >= 0) {
            if constexpr (std::is_trivially_copyable<T>::value) {
                memcpy((void *)data,
                    (void *)(((ArrayWithBucket<T,allocTail,noAllocCount>*)range.ptr)->data + range.index),
                    sizeof(T) * size);
            } else {
                for (i32 i = 0; i < size; i++) {
                    data[i] = range[i];
                }
            }
        } else {
            i32 i = 0;
            for (const T &it : range) {
                data[i] = it;
                i++;
            }
        }
        _SetTerminator();
    }

    ~ArrayWithBucket() {
        _Deinitialize();
    }

    ArrayWithBucket<T, noAllocCount, allocTail>&
    operator=(const ArrayWithBucket &other) {
        Resize(other.size);
        _Copy(other);
        _SetTerminator();
        return *this;
    }

    ArrayWithBucket<T, noAllocCount, allocTail>&
    operator=(ArrayWithBucket &&other) {
        _Deinitialize();
        _Acquire(std::move(other));
        _SetTerminator();
        other._Drop();
        return *this;
    }

    template<i32 otherAllocTail>
    ArrayWithBucket<T, noAllocCount, allocTail>&
    operator=(const Array<T, otherAllocTail> &other) {
        Resize(other.size);
        _Copy(other);
        _SetTerminator();
        return *this;
    }

    template<i32 otherAllocTail>
    ArrayWithBucket<T, noAllocCount, allocTail>&
    operator=(Array<T, otherAllocTail> &&other) {
        _Deinitialize();
        _Acquire(std::move(other));
        _SetTerminator();
        other._Drop();
        return *this;
    }

    ArrayWithBucket<T, noAllocCount, allocTail>&
    operator=(const std::initializer_list<T> &init) {
        Resize(init.size());
        if (size != 0) {
            _Copy(init);
        }
        _SetTerminator();
        return *this;
    }

    ArrayWithBucket<T, noAllocCount, allocTail>&
    operator=(const T *string) {
        Resize(StringLength(string));
        for (i32 i = 0; i < size; i++) {
            data[i] = string[i];
        }
        _SetTerminator();
        return *this;
    }

    bool operator==(const ArrayWithBucket &other) const {
        if (size != other.size) {
            return false;
        }
        for (i32 i = 0; i < size; i++) {
            if (data[i] != other.data[i]) {
                return false;
            }
        }
        return true;
    }

    bool operator==(const Range<T> &other) const {
        if (size != other.size) {
            return false;
        }
        for (i32 i = 0; i < size; i++) {
            if (data[i] != other[i]) {
                return false;
            }
        }
        return true;
    }

    bool operator==(const T *string) const {
        i32 i = 0;
        for (; string[i] != StringTerminators<T>::value && i < size; i++) {
            if (string[i] != data[i]) {
                return false;
            }
        }
        if (i != size) {
            return false;
        }
        return true;
    }

    bool operator!=(const ArrayWithBucket &other) const {
        return !(*this == other);
    }

    bool operator<(const ArrayWithBucket &other) const {
        for (i32 i = 0; i < size && i < other.size; i++) {
            if (data[i] < other.data[i]) return true;
            if (data[i] > other.data[i]) return false;
        }
        return size < other.size;
    }

    const T &operator[](i32 index) const {
#ifndef MEMORY_NO_BOUNDS_CHECKS
        if (index > size) { // Negative values should be large in a u32
            throw std::out_of_range("ArrayWithBucket index is out of bounds");
        }
#endif
        return data[index];
    }

    T &operator[](i32 index) {
#ifndef MEMORY_NO_BOUNDS_CHECKS
        if (index > size) {
            throw std::out_of_range("ArrayWithBucket index is out of bounds");
        }
#endif
        return data[index];
    }

    inline ArrayWithBucket<T, noAllocCount, allocTail> force_inline
    operator+(T &&other) const {
        ArrayWithBucket<T, noAllocCount, allocTail> result(*this);
        result.Append(std::move(other));
        return result;
    }

    inline ArrayWithBucket<T, noAllocCount, allocTail> force_inline
    operator+(const T &other) const {
        ArrayWithBucket<T, noAllocCount, allocTail> result(*this);
        result.Append(other);
        return result;
    }

    inline ArrayWithBucket<T, noAllocCount, allocTail> force_inline
    operator+(const T *string) const {
        ArrayWithBucket<T, noAllocCount, allocTail> result(*this);
        result.Append(string);
        return result;
    }

    inline ArrayWithBucket<T, noAllocCount, allocTail> force_inline
    operator+(ArrayWithBucket &&other) const {
        ArrayWithBucket<T, noAllocCount, allocTail> result(*this);
        result.Append(std::move(other));
        return result;
    }

    inline ArrayWithBucket<T, noAllocCount, allocTail> force_inline
    operator+(const ArrayWithBucket &other) const {
        ArrayWithBucket<T, noAllocCount, allocTail> result(*this);
        result.Append(other);
        return result;
    }

    inline T force_inline
    &operator+=(T &&value) {
        return Append(std::move(value));
    }

    inline T force_inline
    &operator+=(const T &value) {
        return Append(value);
    }

    inline ArrayWithBucket<T, noAllocCount, allocTail>& force_inline
    operator+=(const ArrayWithBucket &other) {
        return Append(other);
    }

    inline ArrayWithBucket<T, noAllocCount, allocTail>& force_inline
    operator+=(ArrayWithBucket &&other) {
        return Append(std::move(other));
    }

    inline ArrayWithBucket<T, noAllocCount, allocTail>& force_inline
    operator+=(const T *string) {
        return Append(string);
    }

    void Reserve(i32 newSize) {
        if (newSize <= allocated || newSize <= noAllocCount-allocTail) {
            return;
        }
        const bool doDelete = allocated != 0;
        allocated = newSize;
        if (size > 0) {
            T *temp = new T[newSize + allocTail];
            if constexpr (std::is_trivially_copyable<T>::value) {
                memcpy((void *)temp, (void *)data, sizeof(T) * size);
            } else {
                for (i32 i = 0; i < size; i++) {
                    temp[i] = std::move(data[i]);
                }
            }
            if (doDelete) {
                delete[] data;
            }
            data = temp;
            _SetTerminator();
            return;
        }
        if (doDelete) {
            delete[] data;
        }
        if (allocated) {
            data = new T[allocated + allocTail];
        } else {
            data = noAllocData;
        }
        _SetTerminator();
    }

    inline void force_inline
    _Grow(i32 minSize) {
        if (minSize > allocated && minSize > noAllocCount-allocTail) {
            i32 growth = minSize + (minSize >> 1) + 4;
            Reserve(growth);
        }
    }

    void Resize(i32 newSize, const T &value) {
        if (newSize == 0) {
            _Deinitialize();
            _Drop();
            _SetTerminator();
            return;
        }
        _Grow(newSize);
        for (i32 i = size; i < newSize; i++) {
            data[i] = value;
        }
        size = newSize;
        _SetTerminator();
    }

    void Resize(i32 newSize) {
        if (newSize == 0) {
            _Deinitialize();
            _Drop();
            _SetTerminator();
            return;
        }
        _Grow(newSize);
        size = newSize;
        _SetTerminator();
    }

    T &Append(const T &value) {
        _Grow(size+1);
        size++;
        _SetTerminator();
        return data[size - 1] = value;
    }

    T &Append(T &&value) {
        _Grow(size+1);
        size++;
        _SetTerminator();
        return data[size - 1] = std::move(value);
    }

    ArrayWithBucket<T, noAllocCount, allocTail>&
    Append(const T *string) {
        i32 newSize = size + StringLength(string);
        Reserve(newSize);
        for (i32 i = size; i < newSize; i++) {
            data[i] = string[i - size];
        }
        size = newSize;
        _SetTerminator();
        return *this;
    }

    inline ArrayWithBucket<T, noAllocCount, allocTail>& force_inline
    Append(const ArrayWithBucket &other) {
        ArrayWithBucket<T, noAllocCount, allocTail> value(other);
        return Append(std::move(value));
    }

    ArrayWithBucket<T, noAllocCount, allocTail>&
    Append(ArrayWithBucket &&other) {
        i32 copyStart = size;
        Resize(size + other.size);
        if constexpr (std::is_trivially_copyable<T>::value) {
            memcpy((void *)(data + copyStart), (void *)other.data, sizeof(T) * other.size);
        } else {
            for (i32 i = copyStart; i < size; i++) {
                data[i] = std::move(other.data[i - copyStart]);
            }
        }
        _SetTerminator();
        other.Clear();
        return *this;
    }

    inline T& force_inline
    Insert(i32 index, const T &value) {
        T val(value);
        return Insert(index, std::move(val));
    }

    T& Insert(i32 index, T &&value) {
#ifndef MEMORY_NO_BOUNDS_CHECKS
        if (index > size) {
            throw std::out_of_range("ArrayWithBucket::Insert index is out of bounds");
        }
#endif
        if (size >= allocated && size >= noAllocCount) {
            const bool doDelete = allocated != 0;
            allocated += (allocated >> 1) + 2;
            T *temp = new T[allocated + allocTail];
            if constexpr (std::is_trivially_copyable<T>::value) {
                if (index > 0) {
                    memcpy((void *)temp, (void *)data, sizeof(T) * index);
                }
                temp[index] = std::move(value);
                if (size - index > 0) {
                    memcpy((void *)(temp + index + 1), (void *)(data + index),
                        sizeof(T) * (size - index));
                }
            } else {
                for (i32 i = 0; i < index; i++) {
                    temp[i] = std::move(data[i]);
                }
                temp[index] = std::move(value);
                for (i32 i = index + 1; i < size + 1; i++) {
                    temp[i] = std::move(data[i - 1]);
                }
            }
            if (doDelete) {
                delete[] data;
            }
            data = temp;
            size++;
            _SetTerminator();
            return data[index];
        }
        // No realloc necessary
        size++;
        for (i32 i = size - 1; i > index; i--) {
            data[i] = std::move(data[i - 1]);
        }
        _SetTerminator();
        return data[index] = std::move(value);
    }

    inline Range<T> force_inline
    Insert(i32 index, const ArrayWithBucket<T, allocTail> &other) {
        ArrayWithBucket<T, allocTail> array(other);
        return Insert(index, std::move(array));
    }

    Range<T> Insert(i32 index, ArrayWithBucket &&other) {
#ifndef MEMORY_NO_BOUNDS_CHECKS
        if (index > size) {
            throw std::out_of_range("ArrayWithBucket::Insert index is out of bounds");
        }
#endif
        if (size == 0) {
            *this = std::move(other);
            return GetRange(0, size);
        }
        if (size+other.size > allocated) {
            const bool doDelete = allocated != 0;
            allocated += (allocated >> 1) + 2;
            if (allocated < size+other.size) allocated = size+other.size;
            T *temp = new T[allocated + allocTail];
            if constexpr (std::is_trivially_copyable<T>::value) {
                if (index > 0) {
                    memcpy((void *)temp, (void *)data, sizeof(T) * index);
                }
                memcpy((void *)(temp + index), (void *)other.data, sizeof(T) * other.size);
                if (size - index > 0) {
                    memcpy((void *)(temp + index + other.size), (void *)(data + index),
                        sizeof(T) * (size - index));
                }
            } else {
                for (i32 i = 0; i < index; i++) {
                    temp[i] = std::move(data[i]);
                }
                for (i32 i = 0; i < other.size; i++) {
                    temp[index+i] = std::move(other[i]);
                }
                for (i32 i = index; i < size; i++) {
                    temp[i+other.size] = std::move(data[i]);
                }
            }
            if (doDelete) {
                delete[] data;
            }
            data = temp;
            size += other.size;
            Range<T> range = GetRange(index, other.size);
            other.Clear();
            _SetTerminator();
            return range;
        }
        size += other.size;
        for (i32 i = size - 1; i >= index+other.size; i--) {
            data[i] = std::move(data[i - other.size]);
        }
        for (i32 i = 0; i < other.size; i++) {
            data[index + i] = std::move(other.data[i]);
        }
        Range<T> range = GetRange(index, other.size);
        other.Clear();
        _SetTerminator();
        return range;
    }

    void Erase(i32 index, const i32 count=1) {
#ifndef MEMORY_NO_BOUNDS_CHECKS
        if (index+count > size && index >= 0) {
            throw std::out_of_range("ArrayWithBucket::Erase index is out of bounds");
        }
#endif
        size -= count;
        if constexpr (std::is_trivially_copyable<T>::value) {
            if (size > index) {
                memmove((void *)(data + index), (void *)(data + index + count),
                    sizeof(T) * (size - index));
            }
        } else {
            for (i32 i = index; i < size; i++) {
                data[i] = data[i + count];
            }
        }
        _SetTerminator();
    }

    ArrayWithBucket<T, noAllocCount, allocTail>&
    Reverse() {
        for (i32 i = 0; i < size / 2; i++) {
            T buf = std::move(data[i]);
            data[i] = std::move(data[size - i - 1]);
            data[size - i - 1] = std::move(buf);
        }
        return *this;
    }

    ArrayIterator<T> begin() const {
        return ArrayIterator<T>(data);
    }
    ArrayIterator<T> end() const {
        return ArrayIterator<T>(data + size);
    }
    T *begin() {
        return data;
    }
    T *end() {
        return data + size;
    }

    inline T& force_inline
    Back() {
        return data[size - 1];
    }

    bool Contains(const T &val) const {
        for (i32 i = 0; i < size; i++) {
            if (val == data[i]) {
                return true;
            }
        }
        return false;
    }

    i32 Count(const T &val) const {
        i32 count = 0;
        for (i32 i = 0; i < size; i++) {
            if (val == data[i]) {
                count++;
            }
        }
        return count;
    }

    Ptr<T> GetPtr(i32 index, bool fromBack = false) {
#ifndef MEMORY_NO_BOUNDS_CHECKS
        if (index >= (size + (i32)fromBack)) {
            throw std::out_of_range("ArrayWithBucket::GetPtr index is out of bounds");
        }
#endif
        if (fromBack) {
            return Ptr<T>((Array<T,0>*)this, index - size);
        } else {
            return Ptr<T>((Array<T,0>*)this, index);
        }
    }

    Range<T> GetRange(i32 index, i32 _size) {
#ifndef MEMORY_NO_BOUNDS_CHECKS
        if (index + _size > size && index >= 0) {
            throw std::out_of_range("ArrayWithBucket::Range index + size is out of bounds");
        }
#endif
        return Range<T>((Array<T,0>*)this, index, _size);
    }
};

template<typename T, i32 noAllocCount, i32 allocTail>
inline ArrayWithBucket<T, noAllocCount, allocTail> force_inline
operator+(ArrayWithBucket<T, noAllocCount, allocTail> &&lhs, ArrayWithBucket<T, noAllocCount, allocTail> &&rhs) {
    ArrayWithBucket<T, noAllocCount, allocTail> out(std::move(lhs));
    out.Append(std::move(rhs));
    return out;
}

} // namespace AzCore

#endif // AZCORE_ARRAYWITHBUCKET_HPP
