/*
    File: String.hpp
    Author: Philip Haynes
*/

#ifndef AZCORE_STRING_HPP
#define AZCORE_STRING_HPP

#include "Array.hpp"

namespace AzCore {

using String = Array<char, 1>;
using WString = Array<char32, 1>;

String operator+(const char *cString, String &&string);
String operator+(const char *cString, const String &string);
WString operator+(const char32 *cString, WString &&string);
WString operator+(const char32 *cString, const WString &string);

String ToString(const u32 &value, i32 base = 10);
String ToString(const u64 &value, i32 base = 10);
String ToString(const u128 &value, i32 base = 10);
String ToString(const i32 &value, i32 base = 10);
String ToString(const i64 &value, i32 base = 10);
String ToString(const i128 &value, i32 base = 10);
String ToString(const f32 &value, i32 base = 10, i32 precision = -1);
String ToString(const f64 &value, i32 base = 10, i32 precision = -1);
String ToString(const f128 &value, i32 base = 10, i32 precision = -1);

f32 StringToF32(String string, i32 base = 10);
f32 WStringToF32(WString string, i32 base = 10);
i64 StringToI64(String string, i32 base = 10);

inline bool operator==(const char *b, const String &a)
{
    return a == b;
}

bool equals(const char *a, const char *b);

// Converts a UTF-8 string to Unicode string
WString ToWString(const char *string);
// Converts a UTF-8 string to Unicode string
WString ToWString(String string);
// Returns how many bytes long a single UTF-8 character is based on the first.
i32 CharLen(const char chr);

inline WString operator+(const WString &wString, const char *cString)
{
    return wString + ToWString(cString);
}
inline WString operator+(const WString &wString, const String &string)
{
    return wString + ToWString(string);
}
inline WString operator+(const char *cString, const WString &wString)
{
    return ToWString(cString) + wString;
}
inline WString operator+(const String string, const WString &wString)
{
    return ToWString(string) + wString;
}

} // namespace AzCore

#endif // AZCORE_STRING_HPP
