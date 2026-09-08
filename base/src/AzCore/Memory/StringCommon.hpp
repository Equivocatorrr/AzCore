/*
	File: StringCommon.hpp
	Author: Philip Haynes
	Common utilities for all Array/String-like types.
*/

#ifndef AZCORE_STRINGCOMMON_HPP
#define AZCORE_STRINGCOMMON_HPP

#include "../BasicTypes.hpp"
#include <cstdint>

namespace AzCore {

/*  struct: StringTerminators
	Author: Philip Haynes
	If you want to use value-terminated strings with Arrays or StringLength, the correct
	string terminator must be declared somewhere in a .cpp file (or in a .hpp for constexpr). char and char32 are already set.  */
template <typename T>
struct StringTerminators {};
/* Macro to easily set a terminator. Must be called from one .hpp file before instantiating StringLength or Array types with terminators.
   Definitions for char and char32 are already set. */
#define AZCORE_STRING_TERMINATOR(TYPE, VAL)\
	namespace AzCore {\
		template<>\
		struct StringTerminators<TYPE> {\
			static constexpr TYPE value = VAL;\
		};\
	} // namespaceAzCore

/*  i32 StringLength(const T *string)
	Author: Philip Haynes
	Finds the length of a value-terminated string. The type T must have an
	associated StringTerminators declared somewhere. */
template <typename T>
constexpr i32 StringLength(const T *string, i32 maxLen=INT32_MAX) {
	i32 length = 0;
	while (string[length] != StringTerminators<T>::value) {
		length++;
		if (length == maxLen) break;
	}
	return length;
}

} // namespace AzCore

AZCORE_STRING_TERMINATOR(char, 0);
AZCORE_STRING_TERMINATOR(char32, 0);

#endif // AZCORE_STRINGCOMMON_HPP
