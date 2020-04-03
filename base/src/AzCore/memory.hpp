/*
    File: memory.hpp
    Author: Philip Haynes
    Includes all the headers in Azcore/Memory and aliases some from the C++ Standard Library.
*/
#ifndef AZCORE_MEMORY_HPP
#define AZCORE_MEMORY_HPP

#ifdef NDEBUG
#define MEMORY_NO_BOUNDS_CHECKS
#endif

#include "Memory/Endian.hpp"
#include "Memory/Array.hpp"
#include "Memory/String.hpp"
#include "Memory/List.hpp"
#include "Memory/ArrayList.hpp"
#include "Memory/BucketArray.hpp"
#include "Time.hpp"

#include <map>
#include <set>
#include <memory>

namespace AzCore {

size_t align(const size_t& size, const size_t& alignment);

template<typename T, typename B>
using Map = std::map<T, B>;

template<typename T>
using Set = std::set<T>;

template<typename T, typename Deleter=std::default_delete<T>>
using UniquePtr = std::unique_ptr<T, Deleter>;

template<typename T>
using SharedPtr = std::shared_ptr<T>;

template<typename T>
using WeakPtr = std::weak_ptr<T>;

Array<char> FileContents(String filename);

template<typename T, i32 allocTail>
Array<Range<T>> SeparateByValues(Array<T, allocTail> *array, const Array<T> &values) {
    Array<Range<T>> result;
    i32 rangeStart = 0;
    for (i32 i = 0; i < array->size; i++) {
        if (values.Contains(array->data[i])) {
            result.Append(array->GetRange(rangeStart, i-rangeStart));
            rangeStart = i+1;
        }
    }
    if (rangeStart < array->size) {
        result.Append(array->GetRange(rangeStart, array->size-rangeStart));
    }
    return result;
}

} // namespace AzCore

#endif // AZCORE_MEMORY_HPP
