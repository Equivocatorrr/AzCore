/*
	File: StringCommon.hpp
	Author: Philip Haynes
	Common utilities for all Array/String-like types.
*/

#ifndef AZCORE_STRINGCOMMON_HPP
#define AZCORE_STRINGCOMMON_HPP

#include "../basictypes.hpp"

namespace AzCore {

/*  struct: StringTerminators
	Author: Philip Haynes
	If you want to use value-terminated strings with Arrays or StringLength, the correct
	string terminator must be declared somewhere in a .cpp file. char and char32 are already set.  */
template <typename T>
struct StringTerminators {
	static const T value;
};
/* Macro to easily set a terminator. Must be called from one and only one .cpp file.
   Definitions for char and char32 are already set. */
#define AZCORE_STRING_TERMINATOR(TYPE, VAL) template <> \
	const TYPE AzCore::StringTerminators<TYPE>::value = VAL

/*  i32 StringLength(const T *string)
	Author: Philip Haynes
	Finds the length of a value-terminated string. The type T must have an
	associated StringTerminators declared somewhere. */
template <typename T>
i32 StringLength(const T *string) {
	i32 length = 0;
	while (string[length] != StringTerminators<T>::value) {
		length++;
	}
	return length;
}

} // namespace AzCore

#endif // AZCORE_STRINGCOMMON_HPP
