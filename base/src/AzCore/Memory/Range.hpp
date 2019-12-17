/*
    File: Range.hpp
    Author: Philip Haynes
*/

#ifndef AZCORE_RANGE_HPP
#define AZCORE_RANGE_HPP

#include "../basictypes.hpp"
#include <stdexcept> // std::out_of_range

namespace AzCore {

template <typename T, i32 allocTail>
struct Array;
template <typename T>
struct ListIndex;
template <typename T>
struct List;
template <typename T>
struct Ptr;

template <typename T>
struct RangeIterator {
    void *ptr;
    i32 iteration;
    RangeIterator() : ptr(nullptr), iteration(-1) {}
    RangeIterator(T *arrayPtr) : ptr(arrayPtr), iteration(-1) {}
    RangeIterator(ListIndex<T> *listIndex, i32 it) : ptr(listIndex), iteration(it) {}
    bool operator!=(const RangeIterator<T> &other) const {
        if (iteration == -1) {
            return ptr != other.ptr;
        } else {
            return iteration != other.iteration;
        }
    }
    void operator++() {
        if (iteration >= 0) {
            ListIndex<T> *listIndex = (ListIndex<T>*)ptr;
            listIndex = listIndex->next;
            iteration++;
        } else {
            T* p = (T*)ptr;
            p++;
            ptr = (void*)p;
        }
    }
    T& operator*() {
        if (iteration >= 0) {
            ListIndex<T> *listIndex = (ListIndex<T>*)ptr;
            return listIndex->value;
        } else {
            return *(T*)ptr;
        }
    }
};

/*  struct: Range
    Author: Philip Haynes
    Using an index and count, points to a range of values from an Array or a List.        */
template <typename T>
struct Range
{
    void *ptr = nullptr;
    i32 index = 0;
    i32 size = 0;
    Range() {}
    template<i32 allocTail>
    Range(Array<T,allocTail> *a, i32 i, i32 s)
    {
        ptr = a;
        index = i;
        size = s;
    }
    Range(List<T> *a, i32 i, i32 s)
    {
        ListIndex<T> *it = a->first;
        for (index = 0; index < i; index++)
        {
            it = it->next;
        }
        ptr = it;
        index = -1;
        size = s;
    }
    template<i32 allocTail>
    void Set(Array<T,allocTail> *a, i32 i, i32 s)
    {
        ptr = a;
        index = i;
        size = s;
    }
    void Set(List<T> *a, i32 i, i32 s)
    {
        ListIndex<T> *it = a->first;
        for (index = 0; index < i; index++)
        {
            it = it->next;
        }
        ptr = it;
        index = -1;
        size = s;
    }
    Ptr<T> GetPtr(const i32 &i)
    {
#ifndef MEMORY_NO_BOUNDS_CHECKS
        if (i >= size)
        {
            throw std::out_of_range("Range index is out of bounds");
        }
#endif
        if (index >= 0)
        {
            return Ptr<T>((Array<T,0> *)ptr, index + i);
        }
        else
        {
            ListIndex<T> *it = (ListIndex<T> *)ptr;
            for (index = 0; index < i; index++)
            {
                it = it->next;
            }
            index = -1;
            return Ptr<T>(&it->value);
        }
    }
    bool Valid() const
    {
        return ptr != nullptr;
    }
    T &operator[](const i32 &i)
    {
#ifndef MEMORY_NO_BOUNDS_CHECKS
        if (i >= size)
        {
            throw std::out_of_range("Range index is out of bounds");
        }
#endif
        if (index >= 0)
        {
            return (*((Array<T,0> *)ptr))[i + index];
        }
        else
        {
            ListIndex<T> *it = (ListIndex<T> *)ptr;
            for (index = 0; index < i; index++)
            {
                it = it->next;
            }
            index = -1;
            return it->value;
        }
    }
    const T &operator[](const i32 &i) const
    {
#ifndef MEMORY_NO_BOUNDS_CHECKS
        if (i >= size)
        {
            throw std::out_of_range("Range index is out of bounds");
        }
#endif
        if (index >= 0)
        {
            return (*((Array<T,0> *)ptr))[i + index];
        }
        else
        {
            ListIndex<T> *it = (ListIndex<T> *)ptr;
            for (i32 ii = 0; ii < i; ii++)
            {
                it = it->next;
            }
            return it->value;
        }
    }

    RangeIterator<T> begin() {
        if (index >= 0) {
            return RangeIterator(&((Array<T,0>*)ptr)->data[index]);
        } else {
            return RangeIterator((ListIndex<T>*)ptr, 0);
        }
    }
    RangeIterator<T> end() {
        if (index >= 0) {
            return RangeIterator(&((Array<T,0>*)ptr)->data[index] + size);
        } else {
            return RangeIterator((ListIndex<T>*)nullptr, size);
        }
    }

    bool Contains(const T &val) const
    {
        if (index >= 0)
        {
            Array<T,0> *array = (Array<T,0>*)ptr;
            for (i32 i = 0; i < size; i++)
            {
                if (val == array->data[i+index])
                    return true;
            }
        }
        else
        {
            ListIndex<T> *it = (ListIndex<T> *)ptr;
            for (i32 i = 0; i < size; i++)
            {
                if (val == it->value)
                    return true;
                it = it->next;
            }
        }
        return false;
    }

    i32 Count(const T &val) const {
        i32 count = 0;
        if (index >= 0)
        {
            Array<T,0> *array = (Array<T,0>*)ptr;
            for (i32 i = 0; i < size; i++)
            {
                if (val == array->data[i+index])
                    count++;
            }
        }
        else
        {
            ListIndex<T> *it = (ListIndex<T> *)ptr;
            for (i32 i = 0; i < size; i++)
            {
                if (val == it->value)
                    count++;
                it = it->next;
            }
        }
        return count;
    }

    bool operator==(Range<T> &other)
    {
        if (size != other.size)
        {
            return false;
        }
        auto myIterator = begin();
        auto myIteratorEnd = end();
        auto otherIterator = other.begin();
        while (myIterator != myIteratorEnd) {
            if (*myIterator != *otherIterator) {
                return false;
            }
            ++myIterator;
            ++otherIterator;
        }
        return true;
    }
};

} // namespace AzCore

#endif // AZCORE_RANGE_HPP
