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
    Range(Array<T,0> *a, i32 i, i32 s)
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
    void Set(Array<T,0> *a, i32 i, i32 s)
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
        if (i >= size)
        {
            throw std::out_of_range("Range index is out of bounds");
        }
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
};

} // namespace AzCore

#endif // AZCORE_RANGE_HPP