/*
    File: Ptr.hpp
    Author: Philip Haynes
*/

#ifndef AZCORE_PTR_HPP
#define AZCORE_PTR_HPP

#include "../basictypes.hpp"

namespace AzCore {

template <typename T, i32 allocTail>
struct Array;

/*  struct: Ptr
    Author: Philip Haynes
    May refer to either an index in an Array or just a raw pointer.     */
template <typename T>
struct Ptr
{
    void *ptr = nullptr;
    i32 index = 0;
    Ptr() {}
    Ptr(T *a)
    {
        ptr = a;
        index = -1;
    }
    Ptr(Array<T,0> *a, i32 i)
    {
        ptr = a;
        index = i;
    }
    void Set(Array<T,0> *a, i32 i)
    {
        ptr = a;
        index = i;
    }
    void Set(T *a)
    {
        ptr = a;
        index = -1;
    }
    bool Valid() const
    {
        return ptr != nullptr;
    }
    bool operator==(T *other) const
    {
        if (index < 0)
        {
            return other == ptr;
        }
        else
        {
            return other == &(*reinterpret_cast<Array<T,0> *>(ptr))[index];
        }
    }
    bool operator!=(T *other) const
    {
        if (index < 0)
        {
            return !(other == ptr);
        }
        else
        {
            return !(other == &(*reinterpret_cast<Array<T,0> *>(ptr))[index]);
        }
    }
    bool operator==(Ptr<T> other) const
    {
        return ptr == other.ptr;
    }
    bool operator!=(Ptr<T> other) const
    {
        return ptr != other.ptr;
    }
    T &operator*()
    {
        if (index < 0)
        {
            return *reinterpret_cast<T *>(ptr);
        }
        else
        {
            return (*reinterpret_cast<Array<T,0> *>(ptr))[index];
        }
    }
    T *operator->()
    {
        if (index < 0)
        {
            return reinterpret_cast<T *>(ptr);
        }
        else
        {
            return &(*reinterpret_cast<Array<T,0> *>(ptr))[index];
        }
    }
    const T &operator*() const
    {
        if (index < 0)
        {
            return *reinterpret_cast<T *>(ptr);
        }
        else
        {
            return (*reinterpret_cast<Array<T,0> *>(ptr))[index];
        }
    }
    const T *operator->() const
    {
        if (index < 0)
        {
            return reinterpret_cast<T *>(ptr);
        }
        else
        {
            return &(*reinterpret_cast<Array<T,0> *>(ptr))[index];
        }
    }
};

} // namespace AzCore

#endif // AZCORE_PTR_HPP