/*
	File: UniquePtr.hpp
	Author: Philip Haynes
	A struct that holds a pointer to a type, implements copy and move,
	and deletes it if it goes out of scope.
*/

#ifndef AZCORE_UNIQUEPTR_HPP
#define AZCORE_UNIQUEPTR_HPP

#include <utility>
#include "../basictypes.hpp"

namespace AzCore {

template<typename T>
struct UniquePtr {
	mutable T* ptr;

	UniquePtr() : ptr(nullptr) {}
	// We claim ownership of the pointer
	inline UniquePtr(T *val) : ptr(val) {}
	inline UniquePtr(const T &val) : ptr(new T(val)) {}
	inline UniquePtr(T &&val) : ptr(new T(std::move(val))) {}
	inline UniquePtr(const UniquePtr<T> &other) {
		if (other.ptr) {
			ptr = new T(*other.ptr);
		} else {
			ptr = nullptr;
		}
	}
	inline UniquePtr(UniquePtr<T> &&other) {
		ptr = other.ptr;
		other.ptr = nullptr;
	}
	inline ~UniquePtr() {
		if (ptr) delete ptr;
	}
	// Let go of the pointer, leaving the responsibility of cleaning it up to someone else.
	[[nodiscard]] T* Release() {
		T* out = ptr;
		ptr = nullptr;
		return out;
	}
	void Clear() {
		if (ptr) {
			delete ptr;
			ptr = nullptr;
		}
	}
	force_inline(T*) RawPtr() {
		return ptr;
	}
	force_inline(const T*) RawPtr() const {
		return ptr;
	}
	// We claim ownership of the pointer
	inline UniquePtr<T>& operator=(T *other) {
		if (ptr) {
			delete ptr;
		}
		ptr = other;
		return *this;
	}
	UniquePtr<T>& operator=(const UniquePtr<T> &other) {
		if (this == &other) return *this;
		if (other.ptr) {
			if (ptr) {
				*ptr = *other.ptr;
			} else {
				ptr = new T(*other.ptr);
			}
		} else {
			if (ptr) {
				delete ptr;
				ptr = nullptr;
			} else {
				// Nothing to do
			}
		}
		return *this;
	}
	UniquePtr<T>& operator=(UniquePtr<T> &&other) {
		if (this == &other) return *this;
		if (other.ptr) {
			if (ptr) {
				delete ptr;
			}
			ptr = other.ptr;
			other.ptr = nullptr;
		} else {
			if (ptr) {
				delete ptr;
			}
			ptr = nullptr;
		}
		return *this;
	}
	T& operator*() {
		if (!ptr) {
			ptr = new T();
		}
		return *ptr;
	}
	T* operator->() {
		if (!ptr) {
			ptr = new T();
		}
		return ptr;
	}
	const T& operator*() const {
		if (!ptr) {
			ptr = new T();
		}
		return *ptr;
	}
	const T* operator->() const {
		if (!ptr) {
			ptr = new T();
		}
		return ptr;
	}

	inline bool operator==(const UniquePtr<T> &other) {
		if (!ptr) return !other.ptr;
		if (!other.ptr) return false;
		return *ptr == *other.ptr;
	}

	inline bool operator!=(const UniquePtr<T> &other) {
		return !operator==(other);
	}
};

} // namespace AzCore

#endif // AZCORE_UNIQUEPTR_HPP
