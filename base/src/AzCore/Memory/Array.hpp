/*
    File: Array.hpp
    Author: Philip Haynes
    Defines a dynamic array template and some associated functions and structures.
*/

#ifndef AZCORE_ARRAY_HPP
#define AZCORE_ARRAY_HPP

#include "../basictypes.hpp"
#include "Ptr.hpp"
#include "Range.hpp"
#include <stdexcept> // std::out_of_range
#include <initializer_list>
#include <type_traits> // std::is_trivially_copyable
#include <cstring> // memcpy

namespace AzCore {

/*  struct: StringTerminators
    Author: Philip Haynes
    If you want to use value-terminated strings with Arrays or StringLength, the correct
    string terminator must be declared somewhere in a .cpp file. char and char32 are already set.  */
template <typename T>
struct StringTerminators {
    static const T value;
};
/* Macro to easily set a terminator. Must be called from one and only one .cpp file.
   Definitions for char and char32 are already set. */
#define AZCORE_STRING_TERMINATOR(TYPE, VAL) template <> \
                                     const TYPE AzCore::StringTerminators<TYPE>::value = VAL

/*  i32 StringLength(const T *string)
    Author: Philip Haynes
    Finds the length of a value-terminated string. The type T must have an
    associated StringTerminators declared somewhere. */
template <typename T>
i32 StringLength(const T *string) {
    i32 length = 0;
    while (string[length] != StringTerminators<T>::value) {
        length++;
    }
    return length;
}

/*  class: ArrayIterator
    Author: Philip Haynes
    Because const correctness can't work without it...      */
template <typename T>
class ArrayIterator {
    T *data;

public:
    ArrayIterator() : data(nullptr) {}
    ArrayIterator(T *d) : data(d) {}
    bool operator!=(const ArrayIterator<T> &other) const {
        return data != other.data;
    }
    const ArrayIterator<T> &operator++() {
        data++;
        return *this;
    }
    const T &operator*() const {
        return *data;
    }
};

/*  struct: Array
    Author: Philip Haynes
    A templated dynamic array which is guaranteed to be 16 bytes on a 64-bit architecture.
    allocTail is the number of elements of type T to be set at the end of the valid data,
    starting at data[size].
    For an allocTail != 0, an associated StringTerminators must be declared for type T. */
template <typename T, i32 allocTail=0>
struct Array {
    T *data;
    i32 allocated;
    i32 size;

    inline void force_inline
    _SetTerminator() {
        if constexpr (allocTail > 1) {
            if (allocated == 0) {
                data = (T *)&StringTerminators<T>::value;
            } else {
                for (i32 i = size; i < size + allocTail; i++) {
                    data[i] = StringTerminators<T>::value;
                }
            }
        } else if constexpr (allocTail == 1) {
            if (allocated == 0) {
                data = (T *)&StringTerminators<T>::value;
            } else {
                data[size] = StringTerminators<T>::value;
            }
        }
    }
    inline void force_inline
    _Initialize(i32 newSize) {
        size = newSize;
        allocated = newSize;
        if (newSize > 0) {
            data = new T[allocated + allocTail];
        } else {
            data = nullptr;
        }
    }
    inline void force_inline
    _Deinitialize() {
        if (allocated != 0) {
            delete[] data;
        }
    }
    inline void force_inline
    _Copy(const Array &other) {
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
        data = nullptr;
        size = 0;
        allocated = 0;
    }
    // Take the allocations and/or values from another Array.
    inline void force_inline
    _Acquire(Array &&other) {
        allocated = other.allocated;
        size = other.size;
        data = other.data;
    }
    void Clear() {
        _Deinitialize();
        _Drop();
        _SetTerminator();
    }

    Array() {
        _Initialize(0);
        _SetTerminator();
    }
    Array(i32 newSize) {
        _Initialize(newSize);
        _SetTerminator();
    }
    Array(i32 newSize, const T &value) {
        _Initialize(newSize);
        for (i32 i = 0; i < size; i++) {
            data[i] = value;
        }
        _SetTerminator();
    }

    inline force_inline
    Array(u32 newSize) : Array((i32)newSize) {}

    inline force_inline
    Array(u32 newSize, const T &value) : Array((i32)newSize, value) {}

    Array(const std::initializer_list<T> &init) {
        _Initialize(init.size());
        _Copy(init);
        _SetTerminator();
    }

    Array(const Array &other) {
        _Initialize(other.size);
        _Copy(other);
        _SetTerminator();
    }
    Array(Array &&other) noexcept {
        _Acquire(std::move(other));
        other._Drop();
        _SetTerminator();
    }

    Array(const T *string) {
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

    Array(const Range<T> &range) {
        _Initialize(range.size);
        if (range.index >= 0) {
            if constexpr (std::is_trivially_copyable<T>::value) {
                memcpy((void *)data,
                    (void *)(((Array<T, allocTail>*)range.ptr)->data + range.index),
                    sizeof(T) * size);
            } else {
                for (i32 i = 0; i < size; i++) {
                    data[i] = range[i];
                }
            }
        } else {
            i32 i = 0;
            for (const auto &it : range) {
                data[i] = it;
                i++;
            }
        }
        _SetTerminator();
    }

    ~Array() {
        _Deinitialize();
    }

    Array<T, allocTail>&
    operator=(const Array &other) {
        Resize(other.size);
        _Copy(other);
        _SetTerminator();
        return *this;
    }

    Array<T, allocTail>&
    operator=(Array &&other) {
        _Deinitialize();
        _Acquire(std::move(other));
        _SetTerminator();
        other._Drop();
        other._SetTerminator();
        return *this;
    }

    Array<T, allocTail>&
    operator=(const std::initializer_list<T> &init) {
        Resize(init.size());
        if (size != 0) {
            _Copy(init);
        }
        _SetTerminator();
        return *this;
    }

    Array<T, allocTail>&
    operator=(const T *string) {
        Resize(StringLength(string));
        for (i32 i = 0; i < size; i++) {
            data[i] = string[i];
        }
        _SetTerminator();
        return *this;
    }

    bool operator==(const Array &other) const {
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

    bool operator!=(const Array &other) const {
        return !(*this == other);
    }

    bool operator<(const Array &other) const {
        for (i32 i = 0; i < size && i < other.size; i++) {
            if (data[i] < other.data[i]) return true;
            if (data[i] > other.data[i]) return false;
        }
        return size < other.size;
    }

    const T &operator[](i32 index) const {
#ifndef MEMORY_NO_BOUNDS_CHECKS
        if (index > size) {
            throw std::out_of_range("Array index is out of bounds");
        }
#endif
        return data[index];
    }

    T &operator[](i32 index) {
#ifndef MEMORY_NO_BOUNDS_CHECKS
        if (index > size) {
            throw std::out_of_range("Array index is out of bounds");
        }
#endif
        return data[index];
    }

    inline Array<T, allocTail> force_inline
    operator+(T &&other) const {
        Array<T, allocTail> result(*this);
        result.Append(std::move(other));
        return result;
    }

    inline Array<T, allocTail> force_inline
    operator+(const T &other) const {
        Array<T, allocTail> result(*this);
        result.Append(other);
        return result;
    }

    inline Array<T, allocTail> force_inline
    operator+(const T *string) const {
        Array<T, allocTail> result(*this);
        result.Append(string);
        return result;
    }

    inline Array<T, allocTail> force_inline
    operator+(Array &&other) const {
        Array<T, allocTail> result(*this);
        result.Append(std::move(other));
        return result;
    }

    inline Array<T, allocTail> force_inline
    operator+(const Array &other) const {
        Array<T, allocTail> result(*this);
        result.Append(other);
        return result;
    }

    inline T& force_inline
    operator+=(T &&value) {
        return Append(std::move(value));
    }

    inline T& force_inline
    operator+=(const T &value) {
        return Append(value);
    }

    inline Array<T, allocTail>& force_inline
    operator+=(const Array &other) {
        return Append(other);
    }

    inline Array<T, allocTail>& force_inline
    operator+=(Array &&other) {
        return Append(std::move(other));
    }

    inline Array<T, allocTail>& force_inline
    operator+=(const T *string) {
        return Append(string);
    }

    void Reserve(i32 newSize) {
        if (newSize <= allocated) {
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
            data = nullptr;
        }
        _SetTerminator();
    }

    inline void _Grow(i32 minSize) {
    if (minSize > allocated) {
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

    Array<T, allocTail> &Append(const T *string) {
        i32 newSize = size + StringLength(string);
        Reserve(newSize);
        for (i32 i = size; i < newSize; i++) {
            data[i] = string[i - size];
        }
        size = newSize;
        _SetTerminator();
        return *this;
    }

    Array<T, allocTail>&
    Append(const Array &other) {
        Array<T, allocTail> value(other);
        return Append(std::move(value));
    }

    Array<T, allocTail>&
    Append(Array &&other) {
        if (size == 0) {
            return *this = std::move(other);
        }
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

    T &Insert(i32 index, const T &value) {
        T val(value);
        return Insert(index, std::move(val));
    }

    T &Insert(i32 index, T &&value) {
#ifndef MEMORY_NO_BOUNDS_CHECKS
        if (index > size) {
            throw std::out_of_range("Array::Insert index is out of bounds");
        }
#endif
        if (size >= allocated) {
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

    Range<T> Insert(i32 index, const Array<T, allocTail> &other) {
        Array<T, allocTail> array(other);
        return Insert(index, std::move(array));
    }

    Range<T> Insert(const i32 index, Array &&other) {
#ifndef MEMORY_NO_BOUNDS_CHECKS
        if (index > size) {
            throw std::out_of_range("Array::Insert index is out of bounds");
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
        for (i32 i = size - 1; i > index; i--) {
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

    void Erase(const i32 index, const i32 count=1) {
#ifndef MEMORY_NO_BOUNDS_CHECKS
        if (index+count > size && index >= 0) {
            throw std::out_of_range("Array::Erase index is out of bounds");
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

    Array<T, allocTail> &Reverse() {
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
    inline T &Back() {
        return data[size - 1];
    }
    inline const T &Back() const {
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
            throw std::out_of_range("Array::GetPtr index is out of bounds");
        }
#endif
        if (fromBack) {
            return Ptr<T>(this, index - size);
        } else {
            return Ptr<T>(this, index);
        }
    }

    Range<T> GetRange(i32 index, i32 _size) {
#ifndef MEMORY_NO_BOUNDS_CHECKS
        if (index + _size > size && index >= 0) {
            throw std::out_of_range("Array::Range index + size is out of bounds");
        }
#endif
        return Range<T>(this, index, _size);
    }
};

template<typename T, i32 allocTail>
inline Array<T, allocTail> force_inline
operator+(Array<T, allocTail> &&lhs, Array<T, allocTail> &&rhs) {
    Array<T, allocTail> out(std::move(lhs));
    out.Append(std::move(rhs));
    return out;
}

} // namespace AzCore

#endif // AZCORE_ARRAY_HPP
