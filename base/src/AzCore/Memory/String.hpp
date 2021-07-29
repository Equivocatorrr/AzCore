/*
    File: String.hpp
    Author: Philip Haynes
*/

#ifndef AZCORE_STRING_HPP
#define AZCORE_STRING_HPP

#include "ArrayWithBucket.hpp"

namespace AzCore {

#define AZCORE_STRING_WITH_BUCKET
#ifdef AZCORE_STRING_WITH_BUCKET
    // Both of these should fit in two cache lines each.
    using String = ArrayWithBucket<char, 8/sizeof(char), 1>;
    using WString = ArrayWithBucket<char32, 8/sizeof(char32), 1>;
#else
    using String = Array<char, 1>;
    using WString = Array<char32, 1>;
#endif

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


inline String ToString(String value) {
    return value;
}

inline String ToString(const char *value) {
    return String(value);
}

inline String ToString(char *value) {
    return String(value);
}

template<typename T>
inline String ToString(Range<T> value) {
    return String(value);
}

template<typename T>
inline String Stringify(T value) {
    return ToString(value);
}
template<typename T, typename... Args>
inline String Stringify(T value, Args... args) {
    return Stringify(value) + Stringify(args...);
}


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
inline char CharToUpper(char c) {
    if (c >= 'a' && c <= 'z') c = c + 'A' - 'a';
    return c;
}

inline WString operator+(const WString &wString, const char *cString) {
    return wString + ToWString(cString);
}
inline WString operator+(const WString &wString, const String &string) {
    return wString + ToWString(string);
}
inline WString operator+(const char *cString, const WString &wString) {
    return ToWString(cString) + wString;
}
inline WString operator+(const String string, const WString &wString) {
    return ToWString(string) + wString;
}
inline bool operator^(const String &lhs, const String &rhs) {
    if (lhs.size != rhs.size) return false;
    for (i32 i = 0; i < lhs.size; i++) {
        char c1 = CharToUpper(lhs[i]);
        char c2 = CharToUpper(rhs[i]);
        if (c1 != c2) return false;
    }
    return true;
}

} // namespace AzCore

#endif // AZCORE_STRING_HPP
