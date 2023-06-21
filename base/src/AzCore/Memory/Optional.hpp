/*
	File: Optional.hpp
	Author: Philip Haynes
*/

#ifndef AZCORE_OPTIONAL_HPP
#define AZCORE_OPTIONAL_HPP

#include "../basictypes.hpp"
#include <utility> // std::move
#include <stdexcept>

namespace AzCore {

template<typename T>
class Optional {
	char bytes[sizeof(T)];
	bool exists;
	// Returns the type-punned bytes without initializing anything
	inline const T& _Value() const {
		return *(const T*)bytes;
	}
	// Returns the type-punned bytes without initializing anything
	inline T& _Value() {
		return *(T*)bytes;
	}
public:
	Optional() : exists(false) {}
	Optional(const T &in) : exists(true) {
		new(bytes) T(in);
	}
	Optional(T &&in) : exists(true) {
		new(bytes) T(std::move(in));
	}
	~Optional() {
		if (exists) {
			T &value = _Value();
			value.~T();
		}
	}
	Optional(const Optional &other) : exists(other.exists) {
		if (exists) {
			new(bytes) T(other._Value());
		}
	}
	Optional(Optional &&other) : exists(other.exists) {
		other.exists = false;
		if (exists) {
			new(bytes) T(std::move(other._Value()));
		}
	}
	Optional& operator=(const T &other) {
		if (exists) {
			_Value() = other;
		} else {
			new(bytes) T(other);
			exists = true;
		}
		return *this;
	}
	Optional& operator=(T &&other) {
		if (exists) {
			_Value() = std::move(other);
		} else {
			new(bytes) T(std::move(other));
			exists = true;
		}
		return *this;
	}
	Optional& operator=(const Optional &other) {
		if (exists) {
			if (other.exists) {
				_Value() = other._Value();
			} else {
				_Value().~T();
			}
		} else {
			if (other.exists) {
				new(bytes) T(other._Value());
			} else {
				// Nothing to do :)
			}
		}
		exists = other.exists;
		return *this;
	}
	Optional& operator=(Optional &&other) {
		if (exists) {
			if (other.exists) {
				_Value() = std::move(other._Value());
				other._Value().~T();
			} else {
				_Value().~T();
			}
		} else {
			if (other.exists) {
				new(bytes) T(std::move(other._Value()));
				other._Value().~T();
			} else {
				// Nothing to do :)
			}
		}
		exists = other.exists;
		other.exists = false;
		return *this;
	}
	inline bool Exists() const {
		return exists;
	}
	// This makes sure the value exists. If you need to check, use Exists().
	T& Value() {
		if (!exists) {
			new(bytes) T();
			exists = true;
		}
		return _Value();
	}
	// This AzAsserts that the value exists. If it doesn't execution will abort.
	const T& ValueOrAssert() const {
		AzAssert(exists, "Optional value does not exist.");
		return _Value();
	}
	T& ValueOrAssert() {
		return *(T*)&std::as_const(*this).ValueOrAssert();
	}
	// This throws a std:runtime_error if the value doesn't exist.
	const T& ValueOrThrow() const {
		if (!exists) throw std::runtime_error("Optional value does not exist.");
		return _Value();
	}
	T& ValueOrThrow() {
		return *(T*)&std::as_const(*this).ValueOrThrow();
	}
};

} // namespace AzCore

#endif // AZCORE_OPTIONAL_HPP