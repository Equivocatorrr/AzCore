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
struct StringTerminators
{
    static const T value;
};
/* Macro to easily set a terminator. Must be called from one and only one .cpp file.
   Definitions for char and char32 are already set. */
#define AZCORE_STRING_TERMINATOR(TYPE, VAL) template <> \
                                     const TYPE AzCore::StringTerminators<TYPE>::value = VAL

/*  i32 StringLength(const T *string)
    Author: Philip Haynes
    Finds the length of a value-terminated string. The type T must have an associated StringTerminators declared somewhere. */
template <typename T>
i32 StringLength(const T *string)
{
    i32 length = 0;
    for (; string[length] != StringTerminators<T>::value; length++)
    {
    }
    return length;
}

/*  class: ArrayIterator
    Author: Philip Haynes
    Because const correctness can't work without it...      */
template <typename T>
class ArrayIterator
{
    T *data;

public:
    ArrayIterator() : data(nullptr) {}
    ArrayIterator(T *d) : data(d) {}
    bool operator!=(const ArrayIterator<T> &other) const
    {
        return data != other.data;
    }
    const ArrayIterator<T> &operator++()
    {
        data++;
        return *this;
    }
    const T &operator*() const
    {
        return *data;
    }
};

/*  struct: Array
    Author: Philip Haynes
    A templated dynamic array which is guaranteed to be 16 bytes on a 64-bit architecture.
    allocTail is the number of elements of type T to be set at the end of the valid data, starting at data[size].
    For an allocTail != 0, an associated StringTerminators must be declared for type T. */
template <typename T, i32 allocTail=0>
struct Array
{
    T *data;
    i32 allocated;
    i32 size;

    void SetTerminator()
    {
        if constexpr (allocTail != 0)
        {
            if (allocated == 0)
            {
                data = (T *)&StringTerminators<T>::value;
            }
            else
            {
                for (i32 i = size; i < size + allocTail; i++)
                {
                    data[i] = StringTerminators<T>::value;
                }
            }
        }
    }

    Array() : data(nullptr), allocated(0), size(0)
    {
        if constexpr (allocTail != 0)
        {
            data = (T *)&StringTerminators<T>::value;
        }
    }
    Array(const i32 newSize) : allocated(newSize), size(newSize)
    {
        if (allocated)
        {
            data = new T[allocated + allocTail];
        }
        else
        {
            data = nullptr;
        }
        SetTerminator();
    }
    Array(const i32 newSize, const T &value) : allocated(newSize), size(newSize)
    {
        if (allocated)
        {
            data = new T[allocated + allocTail];
        }
        else
        {
            data = nullptr;
        }
        for (i32 i = 0; i < size; i++)
        {
            data[i] = value;
        }
        SetTerminator();
    }
    Array(const u32 newSize) : Array((i32)newSize) {}
    Array(const u32 newSize, const T &value) : Array((i32)newSize, value) {}
    Array(const std::initializer_list<T> &init) : allocated(init.size()), size(allocated)
    {
        if (size != 0 || allocTail != 0)
        {
            data = new T[allocated + allocTail];
            i32 i = 0;
            for (const T &val : init)
            {
                data[i++] = val;
            }
        }
        else
        {
            data = nullptr;
        }
        SetTerminator();
    }
    Array(const Array<T, allocTail> &other) : allocated(other.size), size(other.size)
    {
        if (allocated)
        {
            data = new T[allocated + allocTail];
        }
        else
        {
            data = nullptr;
        }
        if constexpr (std::is_trivially_copyable<T>::value)
        {
            memcpy((void *)data, (void *)other.data, sizeof(T) * allocated);
        }
        else
        {
            for (i32 i = 0; i < size; i++)
            {
                data[i] = other.data[i];
            }
        }
        SetTerminator();
    }
    Array(Array<T, allocTail> &&other) noexcept : data(other.data), allocated(other.allocated), size(other.size)
    {
        other.data = nullptr;
        other.size = 0;
        other.allocated = 0;
    }

    Array(const T *string) : allocated(StringLength(string)), size(allocated)
    {
        if (allocated)
        {
            data = new T[allocated + allocTail];
        }
        else
        {
            data = nullptr;
        }
        if constexpr (std::is_trivially_copyable<T>::value)
        {
            memcpy((void *)data, (void *)string, sizeof(T) * allocated);
        }
        else
        {
            for (i32 i = 0; i < size; i++)
            {
                data[i] = string[i];
            }
        }
        SetTerminator();
    }

    Array(const Range<T> &range) : allocated(range.size), size(range.size) {
        if (allocated)
        {
            data = new T[allocated + allocTail];
        }
        else
        {
            data = nullptr;
        }
        if (range.index >= 0) {
            if constexpr (std::is_trivially_copyable<T>::value)
            {
                memcpy((void *)data, (void *)(((Array<T,0>*)range.ptr)->data + range.index), sizeof(T) * allocated);
            }
            else
            {
                for (i32 i = 0; i < size; i++)
                {
                    data[i] = range[i];
                }
            }
        } else {
            for (i32 i = 0; i < size; i++)
            {
                data[i] = range[i];
            }
        }
        SetTerminator();
    }

    ~Array()
    {
        if (allocated != 0)
        {
            delete[] data;
        }
    }

    Array<T, allocTail> &operator=(const Array<T, allocTail> &other)
    {
        Array<T, allocTail> value(other);
        return operator=(std::move(value));
    }

    Array<T, allocTail> &operator=(Array<T, allocTail> &&other)
    {
        if (allocated != 0)
        {
            delete[] data;
        }
        data = other.data;
        size = other.size;
        allocated = other.allocated;
        other.data = nullptr;
        other.size = 0;
        other.allocated = 0;
        return *this;
    }

    Array<T, allocTail> &operator=(const std::initializer_list<T> &init)
    {
        size = init.size();
        if (size == 0 && allocTail == 0)
        {
            return *this;
        }
        if (allocated >= size)
        {
            i32 i = 0;
            for (const T &val : init)
            {
                data[i++] = val;
            }
        }
        else
        {
            // We definitely have to allocate
            const bool doDelete = allocated != 0;
            allocated = size;
            if (doDelete)
            {
                delete[] data;
            }
            if (allocated)
            {
                data = new T[allocated + allocTail];
            }
            else
            {
                data = nullptr;
            }
            i32 i = 0;
            for (const T &val : init)
            {
                data[i++] = val;
            }
        }
        SetTerminator();
        return *this;
    }

    Array<T, allocTail> &operator=(const T *string)
    {
        Resize(StringLength(string));
        for (i32 i = 0; i < size; i++)
        {
            data[i] = string[i];
        }
        SetTerminator();
        return *this;
    }

    bool operator==(const Array<T, allocTail> &other) const
    {
        if (size != other.size)
        {
            return false;
        }
        for (i32 i = 0; i < size; i++)
        {
            if (data[i] != other.data[i])
            {
                return false;
            }
        }
        return true;
    }

    bool operator==(const T *string) const
    {
        i32 i = 0;
        for (; string[i] != StringTerminators<T>::value && i < size; i++)
        {
            if (string[i] != data[i])
            {
                return false;
            }
        }
        if (i != size)
        {
            return false;
        }
        return true;
    }

    bool operator<(const Array<T, allocTail> &other) const {
        for (i32 i = 0; i < size && i < other.size; i++) {
            if (data[i] < other.data[i]) return true;
            if (data[i] > other.data[i]) return false;
        }
        if (size < other.size) return true;
        if (size > other.size) return false;
        return false;
    }

    const T &operator[](const i32 index) const
    {
#ifndef MEMORY_NO_BOUNDS_CHECKS
        if (index > size)
        {
            throw std::out_of_range("Array index is out of bounds");
        }
#endif
        return data[index];
    }

    T &operator[](const i32 index)
    {
#ifndef MEMORY_NO_BOUNDS_CHECKS
        if (index > size)
        {
            throw std::out_of_range("Array index is out of bounds");
        }
#endif
        return data[index];
    }

    Array<T, allocTail> operator+(T &&other) const
    {
        Array<T, allocTail> result(*this);
        result.Append(std::move(other));
        return result;
    }

    Array<T, allocTail> operator+(const T &other) const
    {
        Array<T, allocTail> result(*this);
        result.Append(other);
        return result;
    }

    Array<T, allocTail> operator+(const T *string) const
    {
        Array<T, allocTail> result(*this);
        result.Append(string);
        return result;
    }

    Array<T, allocTail> operator+(Array<T, allocTail> &&other) const
    {
        Array<T, allocTail> result(*this);
        result.Append(std::move(other));
        return result;
    }

    Array<T, allocTail> operator+(const Array<T, allocTail> &other) const
    {
        Array<T, allocTail> result(*this);
        result.Append(other);
        return result;
    }

    inline T &operator+=(T &&value)
    {
        return Append(std::move(value));
    }

    inline T &operator+=(const T &value)
    {
        return Append(value);
    }

    inline Array<T, allocTail> &operator+=(const Array<T, allocTail> &other)
    {
        return Append(other);
    }

    inline Array<T, allocTail> &operator+=(Array<T, allocTail> &&other)
    {
        return Append(std::move(other));
    }

    inline Array<T, allocTail> &operator+=(const T *string)
    {
        return Append(string);
    }

    void Reserve(const i32 newSize)
    {
        if (newSize <= allocated)
        {
            return;
        }
        const bool doDelete = allocated != 0;
        allocated = newSize;
        if (size > 0)
        {
            T *temp = new T[newSize + allocTail];
            if constexpr (std::is_trivially_copyable<T>::value)
            {
                memcpy((void *)temp, (void *)data, sizeof(T) * size);
            }
            else
            {
                for (i32 i = 0; i < size; i++)
                {
                    temp[i] = std::move(data[i]);
                }
            }
            delete[] data;
            data = temp;
            SetTerminator();
            return;
        }
        if (doDelete)
        {
            delete[] data;
        }
        if (allocated)
        {
            data = new T[allocated + allocTail];
        }
        else
        {
            data = nullptr;
        }
        SetTerminator();
    }

    void Resize(const i32 newSize, const T &value)
    {
        if (newSize > allocated)
        {
            i32 growth = (allocated >> 1) + 2;
            Reserve(newSize >= growth ? newSize : growth);
        }
        else if (newSize == 0)
        {
            if (allocated != 0)
            {
                delete[] data;
            }
            data = nullptr;
            allocated = 0;
            size = 0;
        }
        for (i32 i = size; i < newSize; i++)
        {
            data[i] = value;
        }
        size = newSize;
        SetTerminator();
    }

    void Resize(const i32 newSize)
    {
        if (newSize > allocated)
        {
            i32 growth = (allocated >> 1) + 2;
            Reserve(newSize >= growth ? newSize : growth);
        }
        else if (newSize == 0 && allocTail == 0)
        {
            if (allocated != 0)
            {
                delete[] data;
            }
            data = nullptr;
            allocated = 0;
            size = 0;
        }
        size = newSize;
        SetTerminator();
    }

    T &Append(const T &value)
    {
        if (size >= allocated)
        {
            const bool doDelete = allocated != 0;
            allocated += (allocated >> 1) + 2;
            T *temp = new T[allocated + allocTail];
            if (size > 0)
            {
                if constexpr (std::is_trivially_copyable<T>::value)
                {
                    memcpy((void *)temp, (void *)data, sizeof(T) * size);
                }
                else
                {
                    for (i32 i = 0; i < size; i++)
                    {
                        temp[i] = std::move(data[i]);
                    }
                }
            }
            if (doDelete)
            {
                delete[] data;
            }
            data = temp;
        }
        size++;
        SetTerminator();
        return data[size - 1] = value;
    }

    T &Append(T &&value)
    {
        if (size >= allocated)
        {
            const bool doDelete = allocated != 0;
            allocated += (allocated >> 1) + 2;
            T *temp = new T[allocated + allocTail];
            if (size > 0)
            {
                if constexpr (std::is_trivially_copyable<T>::value)
                {
                    memcpy((void *)temp, (void *)data, sizeof(T) * size);
                }
                else
                {
                    for (i32 i = 0; i < size; i++)
                    {
                        temp[i] = std::move(data[i]);
                    }
                }
            }
            if (doDelete)
            {
                delete[] data;
            }
            data = temp;
        }
        size++;
        SetTerminator();
        return data[size - 1] = std::move(value);
    }

    Array<T, allocTail> &Append(const T *string)
    {
        i32 newSize = size + StringLength(string);
        Reserve(newSize);
        for (i32 i = size; i < newSize; i++)
        {
            data[i] = string[i - size];
        }
        size = newSize;
        SetTerminator();
        return *this;
    }

    Array<T, allocTail> &Append(const Array<T, allocTail> &other)
    {
        Array<T, allocTail> value(other);
        return Append(std::move(value));
    }

    Array<T, allocTail> &Append(Array<T, allocTail> &&other)
    {
        i32 copyStart = size;
        Resize(size + other.size);
        if constexpr (std::is_trivially_copyable<T>::value)
        {
            memcpy((void *)(data + copyStart), (void *)other.data, sizeof(T) * other.size);
        }
        else
        {
            for (i32 i = copyStart; i < size; i++)
            {
                data[i] = std::move(other.data[i - copyStart]);
            }
        }
        SetTerminator();
        if (other.allocated != 0)
        {
            delete[] other.data;
        }
        other.data = nullptr;
        other.size = 0;
        other.allocated = 0;
        return *this;
    }

    T &Insert(const i32 index, const T &value)
    {
        T val(value);
        return Insert(index, std::move(val));
    }

    T &Insert(const i32 index, T &&value)
    {
#ifndef MEMORY_NO_BOUNDS_CHECKS
        if (index > size)
        {
            throw std::out_of_range("Array::Insert index is out of bounds");
        }
#endif
        if (size >= allocated)
        {
            const bool doDelete = allocated != 0;
            allocated += (allocated >> 1) + 2;
            T *temp = new T[allocated + allocTail];
            if constexpr (std::is_trivially_copyable<T>::value)
            {
                if (index > 0)
                {
                    memcpy((void *)temp, (void *)data, sizeof(T) * index);
                }
                temp[index] = std::move(value);
                if (size - index > 0)
                {
                    memcpy((void *)(temp + index + 1), (void *)(data + index), sizeof(T) * (size - index));
                }
            }
            else
            {
                for (i32 i = 0; i < index; i++)
                {
                    temp[i] = std::move(data[i]);
                }
                temp[index] = value;
                for (i32 i = index + 1; i < size + 1; i++)
                {
                    temp[i] = std::move(data[i - 1]);
                }
            }
            if (doDelete)
            {
                delete[] data;
            }
            data = temp;
            size++;
            SetTerminator();
            return data[index];
        }
        size++;
        for (i32 i = size - 1; i > index; i--)
        {
            data[i] = std::move(data[i - 1]);
        }
        SetTerminator();
        return data[index] = value;
    }

    void Erase(const i32 index)
    {
#ifndef MEMORY_NO_BOUNDS_CHECKS
        if (index >= size)
        {
            throw std::out_of_range("Array::Erase index is out of bounds");
        }
#endif
        size--;
        if constexpr (std::is_trivially_copyable<T>::value)
        {
            if (size > index)
            {
                memmove((void *)(data + index), (void *)(data + index + 1), sizeof(T) * (size - index));
            }
        }
        else
        {
            for (i32 i = index; i < size - 1; i++)
            {
                data[i] = data[i + 1];
            }
        }
        SetTerminator();
    }

    void Clear()
    {
        if (allocated != 0)
        {
            delete[] data;
        }
        data = nullptr;
        size = 0;
        allocated = 0;
        SetTerminator();
    }

    Array<T, allocTail> &Reverse()
    {
        for (i32 i = 0; i < size / 2; i++)
        {
            T buf = data[i];
            data[i] = data[size - i - 1];
            data[size - i - 1] = buf;
        }
        return *this;
    }

    ArrayIterator<T> begin() const
    {
        return ArrayIterator<T>(data);
    }
    ArrayIterator<T> end() const
    {
        return ArrayIterator<T>(data + size);
    }
    T *begin()
    {
        return data;
    }
    T *end()
    {
        return data + size;
    }
    inline T &Back()
    {
        return data[size - 1];
    }

    bool Contains(const T &val) const
    {
        for (i32 i = 0; i < size; i++)
        {
            if (val == data[i])
            {
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

    Ptr<T> GetPtr(const i32 &index)
    {
#ifndef MEMORY_NO_BOUNDS_CHECKS
        if (index >= size)
        {
            throw std::out_of_range("Array::GetPtr index is out of bounds");
        }
#endif
        return Ptr<T>(this, index);
    }

    Range<T> GetRange(const i32 &index, const i32 &_size)
    {
#ifndef MEMORY_NO_BOUNDS_CHECKS
        if (index + _size > size)
        {
            throw std::out_of_range("Array::Range index + size is out of bounds");
        }
#endif
        return Range<T>(this, index, _size);
    }
};

} // namespace AzCore

#endif // AZCORE_ARRAY_HPP
