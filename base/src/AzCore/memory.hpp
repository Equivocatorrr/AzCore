/*
	File: memory.hpp
	Author: Philip Haynes
	Includes all the headers in AzCore/Memory and aliases some from the C++ Standard Library.
*/
#ifndef AZCORE_MEMORY_HPP
#define AZCORE_MEMORY_HPP

#ifdef NDEBUG
#define MEMORY_NO_BOUNDS_CHECKS
#endif

#include "Memory/Endian.hpp"
#include "Memory/Array.hpp"
#include "Memory/ArrayWithBucket.hpp"
#include "Memory/String.hpp"
#include "Memory/List.hpp"
#include "Memory/ArrayList.hpp"
#include "Memory/Ptr.hpp"
#include "Memory/Range.hpp"
#include "Memory/BucketArray.hpp"
#include "Memory/UniquePtr.hpp"
#include "Memory/BinaryMap.hpp"
#include "Memory/HashMap.hpp"
#include "Memory/BinarySet.hpp"
#include "Memory/HashSet.hpp"
#include "Time.hpp"

#include <memory>

namespace AzCore {

template<typename T>
void Swap(T &a, T &b) {
	T c = std::move(a);
	a = std::move(b);
	b = std::move(c);
}

inline bool IsPowerOfTwo(size_t value) {
	return (value & (value-1)) == 0;
}
size_t align(size_t size, size_t alignment);
size_t alignNonPowerOfTwo(size_t size, size_t alignment);

template<typename T>
using SharedPtr = std::shared_ptr<T>;

template<typename T>
using WeakPtr = std::weak_ptr<T>;

Array<char> FileContents(String filename);

template<typename T, i32 allocTail>
Array<Range<T>> SeparateByValues(Array<T, allocTail> &array, const ArrayWithBucket<T, 16/sizeof(T), allocTail> &values, bool allowEmpty=false) {
	Array<Range<T>> result;
	i32 rangeStart = 0;
	for (i32 i = 0; i < array.size; i++) {
		if (values.Contains(array.data[i])) {
			if (allowEmpty || i-rangeStart > 0) {
				result.Append(array.GetRange(rangeStart, i-rangeStart));
			}
			rangeStart = i+1;
		}
	}
	if (rangeStart < array.size) {
		result.Append(array.GetRange(rangeStart, array.size-rangeStart));
	}
	return result;
}

template<typename T, i32 allocTail, i32 noAllocCount>
Array<Range<T>> SeparateByValues(ArrayWithBucket<T, noAllocCount, allocTail> &array,
		const ArrayWithBucket<T, 16/sizeof(T), allocTail> &values, bool allowEmpty=false) {
	Array<Range<T>> result;
	i32 rangeStart = 0;
	for (i32 i = 0; i < array.size; i++) {
		if (values.Contains(array.data[i])) {
			if (allowEmpty || i-rangeStart > 0) {
				result.Append(array.GetRange(rangeStart, i-rangeStart));
			}
			rangeStart = i+1;
		}
	}
	if (rangeStart < array.size) {
		result.Append(array.GetRange(rangeStart, array.size-rangeStart));
	}
	return result;
}

template<typename T, i32 allocTail=0>
Array<Range<T>> SeparateByValues(Range<T> &range,
		const ArrayWithBucket<T, 16/sizeof(T), allocTail> &values, bool allowEmpty=false) {
	Array<Range<T>> result;
	i32 rangeStart = 0;
	for (i32 i = 0; i < range.size; i++) {
		if (values.Contains(range[i])) {
			if (allowEmpty || i-rangeStart > 0) {
				result.Append(range.SubRange(rangeStart, i-rangeStart));
			}
			rangeStart = i+1;
		}
	}
	if (rangeStart < range.size) {
		result.Append(range.SubRange(rangeStart, range.size-rangeStart));
	}
	return result;
}

template<typename T, i32 allocTail=0>
Array<Range<T>> SeparateByValues(T *array,
		const ArrayWithBucket<T, 16/sizeof(T), allocTail> &values, bool allowEmpty=false) {
	Array<Range<T>> result;
	i32 rangeStart = 0;
	for (i32 i = 0; array[i] != StringTerminators<T>::value; i++) {
		if (values.Contains(array[i])) {
			if (allowEmpty || i-rangeStart > 0) {
				result.Append(Range<T>(&array[rangeStart], i-rangeStart));
			}
			rangeStart = i+1;
		}
	}
	if (array[rangeStart] != StringTerminators<T>::value) {
		result.Append(Range<T>(&array[rangeStart], StringLength(array+rangeStart)));
	}
	return result;
}

template<typename T, i32 allocTail=0>
Array<Range<T>> SeparateByStrings(Array<T, allocTail> &array,
		const ArrayWithBucket<SimpleRange<T>, 16/sizeof(SimpleRange<T>), 0> &strings, bool allowEmpty=false) {
	Array<Range<T>> result;
	i32 rangeStart = 0;
	for (i32 i = 0; i < array.size;) {
		i32 foundLen = 0;
		for (const SimpleRange<T> &r : strings) {
			i32 len = 0;
			while (len < r.size && i+len < array.size && r[len] == array[i+len]) {
				len++;
			}
			if (len == r.size && len > foundLen)
				foundLen = len;
		}
		if (foundLen > 0) {
			if (allowEmpty || i-rangeStart > 0) {
				result.Append(array.GetRange(rangeStart, i-rangeStart));
			}
			i += foundLen;
			rangeStart = i;
		} else {
			i++;
		}
	}
	if (rangeStart < array.size) {
		result.Append(array.GetRange(rangeStart, array.size-rangeStart));
	}
	return result;
}
// TODO: Implement SeparateByStrings for ArrayWithBucket, Range, and raw strings.

// Extract the base 2 exponent directly from the bits.
#if AZCORE_COMPILER_SUPPORTS_128BIT_TYPES
force_inline(i16)
Exponent(f128 value) {
	u128 byteCode;
	memcpy((void *)&byteCode, (void *)&value, sizeof(byteCode));
	i16 exponent = ((byteCode >> 112) & 0x7fff) - 0x3fff;
	return exponent;
}
#endif

// Extract the base 2 exponent directly from the bits.
force_inline(i16)
Exponent(f64 value) {
	u64 byteCode;
	memcpy((void *)&byteCode, (void *)&value, sizeof(byteCode));
	i16 exponent = ((i16)(byteCode >> 52) & 0x7ff) - 0x3ff;
	return exponent;
}

// Extract the base 2 exponent directly from the bits.
force_inline(i16)
Exponent(f32 value) {
	u32 byteCode;
	memcpy((void *)&byteCode, (void *)&value, sizeof(byteCode));
	i16 exponent = ((i16)(byteCode >> 23) & 0xff) - 0x7f;
	return exponent;
}

} // namespace AzCore

#endif // AZCORE_MEMORY_HPP
