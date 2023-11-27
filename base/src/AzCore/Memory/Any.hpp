/*
	File: Any.hpp
	Author: Philip Haynes
	A container that can represent any type in a safe way. Also guarantees that the raw pointer to the contained type doesn't move.
*/

#ifndef AZCORE_ANY_HPP
#define AZCORE_ANY_HPP

#include <type_traits>

#include "../Assert.hpp"
#include "TypeHash.hpp"

namespace AzCore {

struct Any {
	void *data = nullptr;
	typedef void (*fp_Deleter)(void*);
	fp_Deleter deleter = nullptr;
	typedef void (*fp_Copyer)(void *src, void **dst);
	fp_Copyer copyer = nullptr;
	u32 typeHash = 0;
	Any() = default;
	
	inline Any(Any &&other) : data(other.data), deleter(other.deleter), copyer(other.copyer), typeHash(other.typeHash) {
		other.data = nullptr;
		other.deleter = nullptr;
		other.copyer = nullptr;
		other.typeHash = 0;
	}
	
	inline Any(const Any &other) {
		if (other.data) {
			AzAssert(other.copyer != nullptr, "Cannot copy without a copyer");
			other.copyer(other.data, &data);
			deleter = other.deleter;
			copyer = other.copyer;
			typeHash = other.typeHash;
		}
	}
	
	// This should allow you to compare deleters to determine whether the types are the same.
	template <typename T>
	static fp_Deleter MakeDeleter() {
		return [](void *ptr) { delete (T*)ptr; };
	}
	template <typename T>
	static fp_Copyer MakeCopyer() {
		if constexpr (std::is_copy_constructible<T>::value) {
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
		copyer(MakeCopyer<T>()),
		typeHash(TypeHash<T>()) {}
	
	template <typename T>
	Any(const T &value) :
		data(new T(value)),
		deleter(MakeDeleter<T>()),
		copyer(MakeCopyer<T>()),
		typeHash(TypeHash<T>()) {}
	
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
		typeHash = TypeHash<T>();
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
		typeHash = TypeHash<T>();
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
		return typeHash == TypeHash<T>();
	}
};

} // namespace AzCore

#endif // AZCORE_ANY_HPP