/*
	File: Range.hpp
	Author: Philip Haynes
*/

#ifndef AZCORE_RANGE_HPP
#define AZCORE_RANGE_HPP

#include "../basictypes.hpp"
#include <stdexcept> // std::out_of_range
#include "Ptr.hpp"
#include "StringCommon.hpp"
#include "Array.hpp"
#include "List.hpp"

namespace AzCore {

template <typename T, i32 noAllocCount, i32 allocTail>
struct ArrayWithBucket;

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
	const T& operator*() const {
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
struct Range {
	void *ptr = nullptr;
	i32 index = 0;
	i32 size = 0;
	Range() {}
	template<i32 allocTail>
	Range(Array<T, allocTail> *a, i32 i, i32 s) {
		ptr = (void*)a;
		index = i;
		size = s;
	}
	Range(List<T> *a, i32 i, i32 s) {
		ListIndex<T> *it = a->first;
		for (index = 0; index < i; index++) {
			it = it->next;
		}
		ptr = (void*)it;
		index = -1;
		size = s;
	}
	Range(T *raw, i32 s) {
		ptr = (void*)raw;
		index = indexIndicatingRaw;
		size = s;
	}
	inline bool PointsToArray() const {
		return index >= 0;
	}
	inline bool PointsToRaw() const {
		return index == indexIndicatingRaw;
	}
	inline bool PointsToList() const {
		return !PointsToArray() && !PointsToRaw();
	}
	template<i32 allocTail>
	void Set(Array<T,allocTail> *a, i32 i, i32 s) {
		ptr = (void*)a;
		index = i;
		size = s;
	}
	void Set(List<T> *a, i32 i, i32 s) {
		ListIndex<T> *it = a->first;
		for (index = 0; index < i; index++) {
			it = it->next;
		}
		ptr = (void*)it;
		index = -1;
		size = s;
	}
	void Set(T *raw, i32 s) {
		ptr = (void*)raw;
		index = indexIndicatingRaw;
		size = s;
	}
	Ptr<T> GetPtr(i32 i) {
#ifndef MEMORY_NO_BOUNDS_CHECKS
		if (i >= size || i < 0) {
			throw std::out_of_range("Range index is out of bounds");
		}
#endif
		if (index >= 0) {
			return Ptr<T>((Array<T,0> *)ptr, index + i);
		} else if (index == indexIndicatingRaw) {
			return Ptr<T>((T*)ptr);
		} else {
			ListIndex<T> *it = (ListIndex<T> *)ptr;
			for (index = 0; index < i; index++) {
				it = it->next;
			}
			index = -1;
			return Ptr<T>(&it->value);
		}
	}
	Range<T> SubRange(i32 _index, i32 _size) {
#ifndef MEMORY_NO_BOUNDS_CHECKS
		if (_index + _size > size && _index >= 0) {
			throw std::out_of_range("Range::SubRange index + size is out of bounds");
		}
#endif
		if (index >= 0) {
			return Range<T>((Array<T,0> *)ptr, index + _index, _size);
		} else if (index == indexIndicatingRaw) {
			return Range<T>(((T*)ptr) + _index, _size);
		} else {
			ListIndex<T> *it = (ListIndex<T> *)ptr;
			for (index = 0; index < _index; index++) {
				it = it->next;
			}
			index = -1;
			Range<T> newRange;
			newRange.ptr = it;
			newRange.index = -1;
			newRange.size = _size;
			return newRange;
		}
	}
	bool Valid() const {
		return ptr != nullptr;
	}
	T &operator[](i32 i) {
#ifndef MEMORY_NO_BOUNDS_CHECKS
		if (i >= size || i < 0) {
			throw std::out_of_range("Range index is out of bounds");
		}
#endif
		if (index >= 0) {
			return (*((Array<T,0> *)ptr))[i + index];
		} else if (index == indexIndicatingRaw) {
			return ((T*)ptr)[i];
		} else {
			ListIndex<T> *it = (ListIndex<T> *)ptr;
			for (index = 0; index < i; index++) {
				it = it->next;
			}
			index = -1;
			return it->value;
		}
	}
	const T &operator[](i32 i) const {
#ifndef MEMORY_NO_BOUNDS_CHECKS
		if (i >= size || i < 0) {
			throw std::out_of_range("Range index is out of bounds");
		}
#endif
		if (index >= 0) {
			return (*((Array<T,0> *)ptr))[i + index];
		} else if (index == indexIndicatingRaw) {
			return ((T*)ptr)[i];
		} else {
			ListIndex<T> *it = (ListIndex<T> *)ptr;
			for (i32 ii = 0; ii < i; ii++) {
				it = it->next;
			}
			return it->value;
		}
	}

	RangeIterator<T> begin() const {
		if (index >= 0) {
			return RangeIterator(&((Array<T,0>*)ptr)->data[index]);
		} else if (index == indexIndicatingRaw) {
			return RangeIterator((T*)ptr);
		} else {
			return RangeIterator((ListIndex<T>*)ptr, 0);
		}
	}
	RangeIterator<T> end() const {
		if (index >= 0) {
			return RangeIterator(&((Array<T,0>*)ptr)->data[index] + size);
		} else if (index == indexIndicatingRaw) {
			return RangeIterator(((T*)ptr) + size);
		} else {
			return RangeIterator((ListIndex<T>*)nullptr, size);
		}
	}

	bool Contains(const T &val) const {
		for (const T &item : *this) {
			if (val == item)
				return true;
		}
		return false;
	}

	i32 Find(const T &val) const {
		i32 index = 0;
		for (const T &item : *this) {
			if (val == item) {
				return index;
			}
			index++;
		}
		return -1;
	}

	i32 Count(const T &val) const {
		i32 count = 0;
		for (const T &item : *this) {
			if (val == item)
				count++;
		}
		return count;
	}

	bool operator==(const Range<T> &other) const {
		if (size != other.size) {
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

	bool operator==(const T *other) const {
		auto myIterator = begin();
		auto myIteratorEnd = end();
		while (myIterator != myIteratorEnd) {
			if (*other == StringTerminators<T>::value)
				return false;
			if (*myIterator != *other) {
				return false;
			}
			++myIterator;
			++other;
		}
		return *other == StringTerminators<T>::value;
	}

	bool operator<(const Range<T> &other) const {
		auto myIterator = begin();
		auto myIteratorEnd = end();
		auto otherIterator = other.begin();
		auto otherIteratorEnd = other.end();
		while (otherIterator != otherIteratorEnd) {
			if (!(myIterator != myIteratorEnd)) {
				return true;
			}
			if (*myIterator <= *otherIterator) {
				return *myIterator != *otherIterator;
			}
			++myIterator;
			++otherIterator;
		}
		return false;
	}
};

// Like Range above, but with fewer bells and whistles.
// Because sometimes simplicity is best.
template <typename T>
struct SimpleRange {
	T *str;
	i64 size;

	SimpleRange() : str(nullptr), size(0) {}
	SimpleRange(std::nullptr_t) : str(nullptr), size(0) {}
	SimpleRange(T *string, i64 length) : str(string), size(length) {}
	SimpleRange(const T *string) : str((T*)string), size(StringLength(string)) {}
	template<i32 allocTail>
	SimpleRange(Array<T, allocTail> &array) : str(array.data), size(array.size) {}
	template<i32 bucketSize, i32 allocTail>
	SimpleRange(ArrayWithBucket<T, bucketSize, allocTail> &array) : str(array.data), size(array.size) {}
	SimpleRange(Range<T> &range) : size(range.size) {
		if (range.PointsToArray()) {
			str = (*((Array<T,0> *)range.ptr))[0];
		} else if (range.PointsToRaw()) {
			str = (T*)range.ptr;
		} else {
			throw std::runtime_error("SimpleRange doesn't work on Lists");
		}
	}

	SimpleRange<T> SubRange(i64 index, i64 _size) {
#ifndef MEMORY_NO_BOUNDS_CHECKS
		if (index + _size > size && index >= 0) {
			throw std::out_of_range("SimpleRange::SubRange index + size is out of bounds");
		}
#endif
		return SimpleRange<T>(str + index, _size);
	}

	T* begin() {
		return str;
	}
	T* end() {
		return str + size;
	}
	const T* begin() const {
		return str;
	}
	const T* end() const {
		return str + size;
	}

	inline T& operator[](i64 i) {
#ifndef MEMORY_NO_BOUNDS_CHECKS
		if (i >= size || i < 0) {
			throw std::out_of_range("Range index is out of bounds");
		}
#endif
		return str[i];
	}
	inline const T& operator[](i64 i) const {
#ifndef MEMORY_NO_BOUNDS_CHECKS
		if (i >= size || i < 0) {
			throw std::out_of_range("Range index is out of bounds");
		}
#endif
		return str[i];
	}

	inline bool operator==(const SimpleRange<T> other) const {
		if (size != other.size) return false;
		for (i64 i = 0; i < size; i++) {
			if (str[i] != other.str[i]) return false;
		}
		return true;
	}
	inline bool operator==(const T *string) const {
		for (i64 i = 0; i < size; i++) {
			if (str[i] != string[i]) return false;
		}
		return string[size] == StringTerminators<T>::value;
	}

	force_inline(bool)
	operator!=(const SimpleRange<T> other) const {
		return !operator==(other);
	}
	force_inline(bool)
	operator!=(const T *string) const {
		return !operator==(string);
	}

	bool operator<(const SimpleRange<T> &other) const {
		if (size != other.size) return size < other.size;
		for (i64 i = 0; i < size; i++) {
			if (str[i] != other.str[i]) return str[i] < other.str[i];
		}
		return false;
	}

	bool Contains(const T &val) const {
		for (const T &item : *this) {
			if (val == item)
				return true;
		}
		return false;
	}

	i64 Find(const T &val) const {
		for (i64 index = 0; index < size; index++) {
			if (val == str[index])
				return index;
		}
		return -1;
	}

	i64 Count(const T &val) const {
		i64 count = 0;
		for (const T &item : *this) {
			if (val == item)
				count++;
		}
		return count;
	}
};

template<u16 bounds>
constexpr i32 IndexHash(const SimpleRange<char> &in) {
	u32 hash = 0;
	for (char c : in) {
		hash = hash * 31 + c;
	}
	return i32(hash % bounds);
}

} // namespace AzCore

#endif // AZCORE_RANGE_HPP
