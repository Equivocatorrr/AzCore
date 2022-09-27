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
static constexpr i32 indexIndicatingRaw = (i32)0xFFFFFFFF; // Because MSVC is stupid

/*  struct: Ptr
	Author: Philip Haynes
	May refer to an index in an Array relative to
	the start or the end or just a raw pointer.     */
template <typename T>
struct Ptr {
	union {
		void *ptr;
		T *raw;
		Array<T,0> *array;
	};
	i32 index;
	constexpr Ptr(const Ptr<T> &other) : ptr(other.ptr), index(other.index) {}
	constexpr Ptr(Ptr<T> &&other) : ptr(other.ptr), index(other.index) {}
	constexpr Ptr<T>& operator=(const Ptr<T> &other) { ptr = other.ptr; index = other.index; return *this; }
	constexpr Ptr<T>& operator=(Ptr<T> &&other) { ptr = other.ptr; index = other.index; return *this; }
	Ptr() = default;
	constexpr Ptr(T *a) : raw(a), index(indexIndicatingRaw) {}

	template<i32 allocTail>
	constexpr Ptr(Array<T,allocTail> *a, i32 i) :
		array((Array<T,0>*)a), index(i) {}

	template<i32 allocTail>
	void Set(Array<T,allocTail> *a, i32 i) {
		array = (Array<T,0>*)a;
		index = i;
	}
	void Set(T *a) {
		raw = a;
		index = indexIndicatingRaw;
	}
	bool Valid() const {
		if (ptr == nullptr) return false;
		if (index == indexIndicatingRaw) {
			return true;
		} else {
			if (index >= 0) {
				return index < array->size;
			} else {
				return -index <= array->size;
			}
		}
	}
	force_inline(T*) RawPtr() {
		return operator->();
	}
	force_inline(const T*) RawPtr() const {
		return operator->();
	}
	bool operator==(T *other) const {
		if (index == indexIndicatingRaw) {
			return other == raw;
		} else {
			if (index >= 0) {
				return other == &(*array)[index];
			} else {
				return other == &(*array)[array->size+index];
			}
		}
	}
	inline bool operator!=(T *other) const {
		return !operator==(other);
	}
	bool operator==(Ptr<T> other) const {
		return ptr == other.ptr && index == other.index;
	}
	bool operator!=(Ptr<T> other) const {
		return ptr != other.ptr || index != other.index;
	}
	T &operator*() {
		if (index == indexIndicatingRaw) {
			return *raw;
		} else {
			if (index >= 0) {
				return (*array)[index];
			} else {
				return (*array)[array->size+index];
			}
		}
	}
	T *operator->() {
		if (index == indexIndicatingRaw) {
			return raw;
		} else {
			if (index >= 0) {
				return &(*array)[index];
			} else {
				return &(*array)[array->size+index];
			}
		}
	}
	const T &operator*() const {
		if (index == indexIndicatingRaw) {
			return *raw;
		} else {
			if (index >= 0) {
				return (*array)[index];
			} else {
				return (*array)[array->size+index];
			}
		}
	}
	const T *operator->() const {
		if (index == indexIndicatingRaw) {
			return raw;
		} else {
			if (index >= 0) {
				return &(*array)[index];
			} else {
				return &(*array)[array->size+index];
			}
		}
	}
};

} // namespace AzCore

#endif // AZCORE_PTR_HPP
