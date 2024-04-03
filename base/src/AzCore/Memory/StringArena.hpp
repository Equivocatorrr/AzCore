/*
	File: StringArena.hpp
	Author: Philip Haynes
	Allows you to use an Arena for Strings with the constraint that no 2 strings are identical.
	This constraint allows you to do string compares with simple integer compares.
	It comes at the cost of string modification being much slower.
	Ideal for string-keyed dictionaries.
*/

#ifndef STRING_ARENA_HPP
#define STRING_ARENA_HPP

#include "BinaryMap.hpp"
#include "String.hpp"
#include "Range.hpp"
#include "Optional.hpp"

// Quick macro to declare an arena in a header and an AString type that points to it. Must be matched by a call to AZCORE_CREATE_STRING_ARENA_CPP and called inside a namespace.
#define AZCORE_CREATE_STRING_ARENA_HPP()\
		extern AzCore::StringArena localStringArena;\
		using AString = AzCore::ArenaString<&localStringArena>;\
		template <typename T>\
		using AStringMap = AzCore::ArenaStringMap<AString, T>;
// Call this in a .cpp to match a call to AZCORE_CREATE_STRING_ARENA_HPP. Must be called inside the same namespace that AZCORE_CREATE_STRING_ARENA_HPP was.
#define AZCORE_CREATE_STRING_ARENA_CPP_SIZE(PAGE_SIZE)\
		AzCore::StringArena localStringArena(PAGE_SIZE);
// Call this in a .cpp to declare an Arena that only exists in that .cpp (no declaration in a .hpp). Must be called in a namespace.
#define AZCORE_CREATE_STRING_ARENA_LOCAL_SIZE(PAGE_SIZE)\
		AzCore::StringArena localStringArena(PAGE_SIZE);\
		using AString = AzCore::ArenaString<&localStringArena>;\
		template <typename T>\
		using AStringMap = AzCore::ArenaStringMap<AString, T>;
// Call this in a .cpp to match a call to AZCORE_CREATE_STRING_ARENA_HPP. Must be called inside the same namespace that AZCORE_CREATE_STRING_ARENA_HPP was.
#define AZCORE_CREATE_STRING_ARENA_CPP() AZCORE_CREATE_STRING_ARENA_CPP_SIZE(AzCore::StringArena::DEFAULT_PAGE_SIZE)
// Call this in a .cpp to declare an Arena that only exists in that .cpp (no declaration in a .hpp). Must be called in a namespace.
#define AZCORE_CREATE_STRING_ARENA_LOCAL() AZCORE_CREATE_STRING_ARENA_LOCAL_SIZE(AzCore::StringArena::DEFAULT_PAGE_SIZE)

namespace AzCore {

class StringArena {
	// 0 is guaranteed to be the empty string.
	BinaryMap<Str, i32> map;
	Array<Str> strings;
	// Strings are 0-terminated in memory, so you can use them like c-strings if you must.
	Array<Array<char>> arenas;
	i32 PAGE_SIZE;
	// Reserves string.size+1 and returns the page
	Array<char>& ReserveMemory(Str string);
public:
	constexpr static i32 DEFAULT_PAGE_SIZE=1024*64;
	StringArena(i32 pageSize=DEFAULT_PAGE_SIZE);
	i32 GetID(const Str string);
	inline const Str GetString(i32 id) const {
		return strings[id];
	}
};

extern StringArena genericStringArena;

template<StringArena *arena=&genericStringArena>
class ArenaString {
	template<class AString_t, typename T>
	friend class ArenaStringMap;
	template<class AString_t, typename T>
	friend class ArenaStringMapIterator;
	i32 id;
public:
	constexpr ArenaString() : id(0) {}
	constexpr ArenaString(Str other) : id(arena->GetID(other)) {}
	constexpr ArenaString(const char *other) : id(arena->GetID(other)) {}
	constexpr ArenaString(const String &other) : id(arena->GetID(other)) {}
	constexpr bool operator==(ArenaString other) const {
		return id == other.id;
	}
	constexpr bool operator==(Str other) const {
		return GetString() == other;
	}
	constexpr bool operator==(const char *other) const {
		return GetString() == other;
	}
	constexpr bool operator==(const String &other) const {
		return GetString() == other;
	}
	constexpr bool operator!=(ArenaString other) const {
		return id != other.id;
	}
	// For binary trees
	constexpr bool operator<(ArenaString other) const {
		return id < other.id;
	}
	constexpr ArenaString& operator=(ArenaString other) {
		id = other.id;
		return *this;
	}
	constexpr ArenaString& operator=(Str other) {
		id = arena->GetID(other);
		return *this;
	}
	constexpr ArenaString& operator=(const String &other) {
		id = arena->GetID(other);
		return *this;
	}
	constexpr ArenaString& operator=(const char *string) {
		return operator=(Str(string));
	}
	constexpr operator const Str() const {
		return GetString();
	}
	constexpr const Str GetString() const {
		return arena->GetString(id);
	}
	// NOTE: Maybe we don't want direct mutators like this. Ideally any String manipulation should be done with String, and the result should THEN be converted to an ArenaString, so we don't have a bunch of intermediate strings in our Arena.
	/*
	constexpr ArenaString operator+(String rhs) const {
		const String &lhs = arena->GetString(id);
		String newString = lhs + rhs;
		ArenaString result;
		result.id = arena->GetID(std::move(newString));
		return result;
	}
	constexpr ArenaString& operator+=(String rhs) {
		const String &lhs = arena->GetString(id);
		String newString = lhs + rhs;
		id = arena->GetID(std::move(newString));
		return *this;
	}
	constexpr ArenaString operator+(ArenaString other) const {
		const String &rhs = arena->GetString(other.id);
		return operator+(rhs);
	}
	constexpr ArenaString operator+=(ArenaString other) {
		const String &rhs = arena->GetString(other.id);
		return operator+=(rhs);
	}
	*/
};

template<StringArena *arena>
inline void AppendToString(String& string, ArenaString<arena> in) {
	AppendToString(string, in.GetString());
}

template<class AString_t, typename T>
class ArenaStringMapIterator {
	AString_t key;
	Optional<T> *it, *end;
public:
	ArenaStringMapIterator(Optional<T> *inIt, Optional<T> *inEnd) : key(), it(inIt), end(inEnd) {}
	struct Node {
		AString_t key;
		T *value;
	};
	bool operator!=(ArenaStringMapIterator other) const {
		return it != other.it;
	}
	Node operator*() {
		return {key, &it->Value()};
	}
	Node operator*() const {
		return {key, &it->Value()};
	}
	void operator++() {
		do {
			key.id++;
			it++;
		} while (!it->Exists() && it != end);
	}
};

template<class AString_t, typename T>
class ArenaStringMap {
	Array<Optional<T>> values;
public:
	ArenaStringMap() = default;
	ArenaStringMap(const ArenaStringMap&) = default;
	ArenaStringMap(ArenaStringMap&&) = default;
	ArenaStringMap& operator=(const ArenaStringMap&) = default;
	ArenaStringMap& operator=(ArenaStringMap&&) = default;
	ArenaStringMap(std::initializer_list<std::pair<AString_t, T>> init) {
		i32 maxIndex = 0;
		for (const auto &it : init) {
			if (it.first.id > maxIndex) maxIndex = it.first.id;
		}
		values.Resize(maxIndex+1);
		for (const auto &it : init) {
			values[it.first.id] = it.second;
		}
	}
	T& operator[](AString_t key) {
		if (key.id >= values.size) values.Resize(key.id+1);
		Optional<T> &opt = values[key.id];
		if (!opt.Exists()) {
			opt = T();
		}
		return opt.Value();
	}
	T& ValueOf(AString_t key, const T &defaultValue) {
		if (key.id >= values.size) {
			values.Resize(key.id+1);
		}
		Optional<T> &opt = values[key.id];
		if (!opt.Exists()) {
			opt = defaultValue;
		}
		return opt.Value();
	}
	T& ValueOf(AString_t key, T &&defaultValue) {
		if (key.id >= values.size) {
			values.Resize(key.id+1);
		}
		Optional<T> &opt = values[key.id];
		if (!opt.Exists()) {
			opt = std::move(defaultValue);
		}
		return opt.Value();
	}
	bool Exists(AString_t key) const {
		if (key.id >= values.size) return false;
		return values[key.id].Exists();
	}
	ArenaStringMapIterator<AString_t, T> begin() {
		if (values.size == 0) return {nullptr, nullptr};
		ArenaStringMapIterator<AString_t, T> result{&values[0], &values.Back()+1};
		if (!values[0].Exists()) ++result;
		return result;
	}
	ArenaStringMapIterator<AString_t, T> end() {
		if (values.size == 0) return {nullptr, nullptr};
		ArenaStringMapIterator<AString_t, T> result{&values.Back()+1, &values.Back()+1};
		return result;
	}
	ArenaStringMapIterator<AString_t, const T> begin() const {
		if (values.size == 0) return{nullptr, nullptr};
		ArenaStringMapIterator<AString_t, const T> result{&values[0], &values.Back()+1};
		if (!values[0].Exists()) ++result;
		return result;
	}
	ArenaStringMapIterator<AString_t, const T> end() const {
		if (values.size == 0) return {nullptr, nullptr};
		ArenaStringMapIterator<AString_t, const T> result{&values.Back()+1, &values.Back()+1};
		return result;
	}
};

} // namespace AzCore

#endif // STRING_ARENA_HPP