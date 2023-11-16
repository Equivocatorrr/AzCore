/*
	File: Any.hpp
	Author: Philip Haynes
	A container that can represent any type in a safe way. Also guarantees that the raw pointer to the contained type doesn't move.
*/

#ifndef AZCORE_ANY_HPP
#define AZCORE_ANY_HPP

#include <type_traits>

#include "../Assert.hpp"

namespace AzCore {

struct Any {
	void *data = nullptr;
	typedef void (*fp_Deleter)(void*);
	fp_Deleter deleter = nullptr;
	typedef void (*fp_Copyer)(void *src, void **dst);
	fp_Copyer copyer = nullptr;
	Any() = default;
	
	inline Any(Any &&other) : data(other.data), deleter(other.deleter) {
		other.data = nullptr;
		other.deleter = nullptr;
	}
	
	inline Any(const Any &other) {
		if (other.data) {
			AzAssert(other.copyer != nullptr, "Cannot copy without a copyer");
			other.copyer(other.data, &data);
			deleter = other.deleter;
			copyer = other.copyer;
		}
	}
	
	// This should allow you to compare deleters to determine whether the types are the same.
	template <typename T>
	static fp_Deleter MakeDeleter() {
		return [](void *ptr) { delete (T*)ptr; };
	}
	template <typename T>
	static fp_Copyer MakeCopyer() {
		if constexpr (std::is_copy_assignable<T>::value) {
			return [](void *src, void **dst) {
				(*dst) = new T(*(const T*)src);
			};
		} else {
			return nullptr;
		}
	}
	
	template <typename T>
	Any(T &&value) :
		data(new T(std::move(value))),
		deleter(MakeDeleter<T>()),
		copyer(MakeCopyer<T>()) {}
	
	template <typename T>
	Any(const T &value) :
		data(new T(value)),
		deleter(MakeDeleter<T>()),
		copyer(MakeCopyer<T>()) {}
	
	template <typename T>
	Any& operator=(T &&value) {
		if (data) {
			if (IsType<T>()) {
				*(T*)data = std::move(value);
				return *this;
			} else {
				deleter(data);
			}
		}
		data = new T(std::move(value));
		deleter = MakeDeleter<T>();
		copyer = MakeCopyer<T>();
		return *this;
	}
	
	template <typename T>
	Any& operator=(const T &value) {
		if (data) {
			if (IsType<T>()) {
				*(T*)data = value;
				return *this;
			} else {
				deleter(data);
			}
		}
		data = new T(value);
		deleter = MakeDeleter<T>();
		copyer = MakeCopyer<T>();
		return *this;
	}
	
	inline ~Any() {
		if (data) {
			AzAssert(nullptr != deleter, "We have data but no deleter!");
			deleter(data);
		}
	}
	
	template <typename T>
	T& Get() {
		AzAssert(nullptr != data, "Trying to Get() data that doesn't exist");
		AzAssert(IsType<T>(), "Trying to Get() with a type that doesn't match what we are");
		return *(T*)data;
	}
	
	template <typename T>
	const T& Get() const {
		AzAssert(nullptr != data, "Trying to Get() data that doesn't exist");
		AzAssert(IsType<T>(), "Trying to Get() with a type that doesn't match what we are");
		return *(T*)data;
	}
	
	template <typename T>
	bool IsType() const {
		// TODO: Confirm that this actually works with multiple translation units.
		return deleter == MakeDeleter<T>();
	}
};

} // namespace AzCore

#endif // AZCORE_ANY_HPP