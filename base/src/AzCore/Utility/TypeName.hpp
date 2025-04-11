/*
	File: TypeName.hpp
	Author: Philip Haynes
	Template type names to Str
*/

#ifndef AZCORE_UTILITY_TYPENAME_HPP
#define AZCORE_UTILITY_TYPENAME_HPP

#include "../Memory/Range.hpp"
#include "Template.hpp"

namespace AzCore {

constexpr bool StartsWith(Range<char> string, Range<char> test) {
	if (string.size < test.size) return false;
	return string.SubRange(0, test.size) == test;
}

// cuts the first count chars off of the beginning of string
constexpr void RemoveFromBeginning(Range<char> &string, i64 count) {
	string.data += count;
	string.size -= count;
}

// Checks if string starts with test, and if it does, cuts that part off
constexpr void RemoveFromBeginning(Range<char> &string, Range<char> test) {
	if (StartsWith(string, test)) {
		RemoveFromBeginning(string, test.size);
	}
}

template <typename T>
constexpr auto TypeName() {
	Range<char> name, prefix, suffix;
	name = AZCORE_PRETTY_FUNCTION;
	#ifdef __clang__
		prefix = "auto AzCore::TypeName() [T = ";
		suffix = "]";
	#elif defined(__GNUC__)
		prefix = "constexpr auto AzCore::TypeName() [with T = ";
		suffix = "]";
	#elif defined(_MSC_VER)
		prefix = "auto __cdecl AzCore::TypeName<";
		suffix = ">(void)";
	#endif
	return name.SubRange(prefix.size, name.size - prefix.size - suffix.size);
}

template<typename T>
constexpr auto TypeNameBase() {
	return TypeName<az::remove_cvref_t<T>>();
}

template<typename T>
constexpr auto TypeNameShort() {
	Range<char> name = TypeNameBase<T>();
	RemoveFromBeginning(name, "struct ");
	i64 i = name.size-1;
	for (; i > 0; i--) {
		if (name[i] == ':') break;
	}
	if (i != 0) {
		RemoveFromBeginning(name, i+1);
	}
	return name;
}

} // namespace AzCore

#endif // AZCORE_UTILITY_TYPENAME_HPP