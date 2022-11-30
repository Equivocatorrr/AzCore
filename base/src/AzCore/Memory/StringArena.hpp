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

// Quick macro to declare an arena in a header and an AString type that points to it. Must be matched by a call to AZCORE_CREATE_STRING_ARENA_CPP and called inside a namespace.
#define AZCORE_CREATE_STRING_ARENA_HPP()\
		extern AzCore::StringArena localStringArena;\
		using AString = AzCore::ArenaString<&localStringArena>;
// Call this in a .cpp to match a call to AZCORE_CREATE_STRING_ARENA_HPP. Must be called inside the same namespace that AZCORE_CREATE_STRING_ARENA_HPP was.
#define AZCORE_CREATE_STRING_ARENA_CPP_SIZE(PAGE_SIZE)\
		AzCore::StringArena localStringArena(PAGE_SIZE);
// Call this in a .cpp to declare an Arena that only exists in that .cpp (no declaration in a .hpp). Must be called in a namespace.
#define AZCORE_CREATE_STRING_ARENA_LOCAL_SIZE(PAGE_SIZE)\
		AzCore::StringArena localStringArena(PAGE_SIZE);\
		using AString = AzCore::ArenaString<&localStringArena>;
// Call this in a .cpp to match a call to AZCORE_CREATE_STRING_ARENA_HPP. Must be called inside the same namespace that AZCORE_CREATE_STRING_ARENA_HPP was.
#define AZCORE_CREATE_STRING_ARENA_CPP() AZCORE_CREATE_STRING_ARENA_CPP_SIZE(StringArena::DEFAULT_PAGE_SIZE)
// Call this in a .cpp to declare an Arena that only exists in that .cpp (no declaration in a .hpp). Must be called in a namespace.
#define AZCORE_CREATE_STRING_ARENA_LOCAL() AZCORE_CREATE_STRING_ARENA_LOCAL_SIZE(StringArena::DEFAULT_PAGE_SIZE)

namespace AzCore {

class StringArena {
	// 0 is guaranteed to be the empty string.
	BinaryMap<SimpleRange<char>, i32> map;
	Array<SimpleRange<char>> strings;
	// Strings are 0-terminated in memory, so you can use them like c-strings if you must.
	Array<Array<char>> arenas;
	i32 PAGE_SIZE;
	// Reserves string.size+1 and returns the page
	Array<char>& ReserveMemory(SimpleRange<char> string);
	constexpr static i32 DEFAULT_PAGE_SIZE=1024*64;
public:
	StringArena(i32 pageSize=DEFAULT_PAGE_SIZE);
	i32 GetID(const SimpleRange<char> string);
	inline const SimpleRange<char> GetString(i32 id) const {
		return strings[id];
	}
};

extern StringArena genericStringArena;

template<StringArena *arena=&genericStringArena>
class ArenaString {
	i32 id;
public:
	constexpr ArenaString() : id(0) {}
	constexpr ArenaString(const SimpleRange<char> other) : id(arena->GetID(other)) {}
	constexpr bool operator==(ArenaString other) {
		return id == other.id;
	}
	constexpr bool operator==(SimpleRange<char> other) {
		return GetString() == other;
	}
	constexpr bool operator!=(ArenaString other) {
		return id != other.id;
	}
	// For binary trees
	constexpr bool operator<(ArenaString other) {
		return id < other.id;
	}
	constexpr ArenaString& operator=(ArenaString other) {
		id = other.id;
		return *this;
	}
	constexpr ArenaString& operator=(String other) {
		id = arena->GetID(other);
		return *this;
	}
	constexpr operator const SimpleRange<char>() {
		return GetString();
	}
	constexpr const SimpleRange<char> GetString() {
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

} // namespace AzCore

#endif // STRING_ARENA_HPP