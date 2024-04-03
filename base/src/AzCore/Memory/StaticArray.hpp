/*
	File: StaticArray.hpp
	Author: Philip Haynes
*/

#ifndef AZCORE_STATICARRAY_HPP
#define AZCORE_STATICARRAY_HPP

#include "../basictypes.hpp"
#include "../Assert.hpp"
#include "TemplateForwardDeclares.hpp"
#include <stdexcept> // std::out_of_range
#include <initializer_list>
#include <type_traits> // std::is_trivially_copyable
#include <cstring>     // memcpy

namespace AzCore {

// NOTE: This is so BigInt can fit in a single cache line.
#pragma pack(1)

/*  struct: StaticArray
	Author: Philip Haynes
	An array with a fixed-sized memory pool and an associated dynamic size. */
template <typename T, i32 count>
struct StaticArray {
	T data[count];
	i32 size;

	StaticArray() : size(0) {}
	StaticArray(i32 newSize) : size(newSize) {
		AzAssert(size <= count, "StaticArray initialized with a size bigger than count");
	}
	StaticArray(i32 newSize, const T &value) : size(newSize) {
		AzAssert(size <= count, "StaticArray initialized with a size bigger than count");
		for (i32 i = 0; i < size; i++) {
			data[i] = value;
		}
	}
	StaticArray(u32 newSize) : StaticArray((i32)newSize) {}
	StaticArray(u32 newSize, const T &value) : StaticArray((i32)newSize, value) {}
	StaticArray(const std::initializer_list<T> &init) : size(init.size()) {
		AzAssert(size <= count, "StaticArray initialized with a size bigger than count");
		i32 i = 0;
		for (const T &val : init) {
			data[i++] = val;
		}
	}
	StaticArray(const T *string) : size(StringLength(string)) {
		AzAssert(size <= count, "StaticArray initialized with a size bigger than count");
		if constexpr (std::is_trivially_copyable<T>::value) {
			memcpy((void *)data, (void *)string, sizeof(T) * size);
		} else {
			for (i32 i = 0; i < size; i++) {
				data[i] = string[i];
			}
		}
	}
	StaticArray(const SimpleRange<T> &range) : size(range.size) {
		AzAssert(size <= count, "StaticArray initialized with a size bigger than count");
		if constexpr (std::is_trivially_copyable<T>::value) {
			memcpy((void *)data, (void *)range.str, sizeof(T) * size);
		} else {
			for (i32 i = 0; i < size; i++) {
				data[i] = range[i];
			}
		}
	}

	StaticArray<T, count> &operator=(const std::initializer_list<T> &init) {
		size = init.size();
		AzAssert(size <= count, "StaticArray assigned with a size bigger than count");
		if (size == 0) {
			return *this;
		}
		i32 i = 0;
		for (const T &val : init) {
			data[i++] = val;
		}
		return *this;
	}

	StaticArray<T, count> &operator=(const T *string) {
		size = StringLength(string);
		AzAssert(size <= count, "StaticArray assigned with a size bigger than count");
		if constexpr (std::is_trivially_copyable<T>::value) {
			memcpy((void *)data, (void *)string, sizeof(T) * size);
		} else {
			for (i32 i = 0; i < size; i++) {
				data[i] = string[i];
			}
		}
		return *this;
	}

	StaticArray<T, count> &operator=(const SimpleRange<T> &range) {
		size = range.size;
		AzAssert(size <= count, "StaticArray assigned with a size bigger than count");
		if constexpr (std::is_trivially_copyable<T>::value) {
			memcpy((void *)data, (void *)range.str, sizeof(T) * size);
		} else {
			for (i32 i = 0; i < size; i++) {
				data[i] = range[i];
			}
		}
		return *this;
	}

	bool operator==(const StaticArray<T, count> &other) const {
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

	bool operator==(const SimpleRange<T> &other) const {
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
		return i == size;
	}

	bool Contains(const T &val, i32 *dstIndex=nullptr) const {
		for (i32 i = 0; i < size; i++) {
			if (val == data[i]) {
				if (dstIndex) *dstIndex = i;
				return true;
			}
		}
		return false;
	}

	inline const T &operator[](i32 index) const {
		AzAssert(index < size && index >= 0, "StaticArray index is out of bounds");
		return data[index];
	}

	inline T &operator[](i32 index) {
		AzAssert(index < size && index >= 0, "StaticArray index is out of bounds");
		return data[index];
	}

	StaticArray<T, count> operator+(const T &other) const {
		StaticArray<T, count> result(*this);
		result.Append(other);
		return result;
	}

	StaticArray<T, count> operator+(const T *string) const {
		StaticArray<T, count> result(*this);
		result.Append(string);
		return result;
	}

	StaticArray<T, count> operator+(const StaticArray<T, count> &other) const {
		StaticArray<T, count> result(*this);
		result.Append(other);
		return result;
	}

	inline T &operator+=(const T &value) {
		return Append(value);
	}

	inline T &operator+=(const T &&value) {
		return Append(std::move(value));
	}

	inline StaticArray<T, count> &operator+=(const StaticArray<T, count> &other) {
		return Append(other);
	}

	inline StaticArray<T, count> &operator+=(const T *string) {
		return Append(string);
	}

	inline void Resize(i32 newSize, const T &value) {
		AzAssert(newSize <= count, "StaticArray Resized bigger than count");
		for (i32 i = size; i < newSize; i++) {
			data[i] = value;
		}
		size = newSize;
	}

	inline void Resize(i32 newSize) {
		AzAssert(newSize <= count, "StaticArray Resized bigger than count");
		size = newSize;
	}

	inline T &Append(const T &value) {
		AzAssert(size < count, "StaticArray Single-Append would overfill");
		return data[size++] = value;
	}

	inline T &Append(T &&value) {
		AzAssert(size < count, "StaticArray Single-Append would overfill");
		return data[size++] = std::move(value);
	}

	StaticArray<T, count> &Append(const T *string) {
		i32 newSize = size + StringLength(string);
		AzAssert(newSize <= count, "StaticArray C-string Append would overfill");
		for (i32 i = size; i < newSize; i++) {
			data[i] = string[i - size];
		}
		size = newSize;
		return *this;
	}

	StaticArray<T, count> &Append(const StaticArray<T, count> &other) {
		i32 copyStart = size;
		size += other.size;
		AzAssert(size <= count, "StaticArray Append would overfill");
		if constexpr (std::is_trivially_copyable<T>::value) {
			memcpy((void *)(data + copyStart), (void *)other.data, sizeof(T) * other.size);
		} else {
			for (i32 i = copyStart; i < size; i++) {
				data[i] = other.data[i - copyStart];
			}
		}
		return *this;
	}

	T &Insert(i32 index, const T &value) {
		T val(value);
		return Insert(index, std::move(val));
	}

	T &Insert(i32 index, T &&value) {
		AzAssert(index <= size && index >= 0, "StaticArray::Insert index is out of bounds");
		AzAssert(size < count, "StaticArray::Insert would overfill");
		for (i32 i = size++; i > index; i--) {
			data[i] = std::move(data[i - 1]);
		}
		return data[index] = value;
	}

	void Erase(i32 index) {
		AzAssert(index < size && index >= 0, "StaticArray::Erase index is out of bounds");
		size--;
		if constexpr (std::is_trivially_copyable<T>::value) {
			if (size > index) {
				memmove((void *)(data + index), (void *)(data + index + 1), sizeof(T) * (size - index));
			}
		} else {
			for (i32 i = index; i < size - 1; i++) {
				data[i] = std::move(data[i + 1]);
			}
		}
	}

	inline void Clear() {
		size = 0;
	}

	StaticArray<T, count> &Reverse() {
		for (i32 i = 0; i < size / 2; i++) {
			T buf = std::move(data[i]);
			data[i] = std::move(data[size - i - 1]);
			data[size - i - 1] = std::move(buf);
		}
		return *this;
	}

	T* begin() {
		return data;
	}
	T* end() {
		return data + size;
	}
	const T* begin() const {
		return data;
	}
	const T* end() const {
		return data + size;
	}

	inline T &Back() {
		AzAssert(size > 0, "StaticArray::Back() called on empty array!");
		return data[size - 1];
	}

	inline const T &Back() const {
		AzAssert(size > 0, "StaticArray::Back() called on empty array!");
		return data[size - 1];
	}
};

#pragma pack()

} // namespace AzCore

#endif // AZCORE_STATICARRAY_HPP
