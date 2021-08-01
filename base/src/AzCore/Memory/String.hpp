/*
    File: String.hpp
    Author: Philip Haynes
*/

#ifndef AZCORE_STRING_HPP
#define AZCORE_STRING_HPP

#include "ArrayWithBucket.hpp"

namespace AzCore {

size_t align(size_t size, size_t alignment);
size_t alignNonPowerOfTwo(size_t size, size_t alignment);

#define AZCORE_STRING_WITH_BUCKET
#ifdef AZCORE_STRING_WITH_BUCKET
    using String = ArrayWithBucket<char, 16/sizeof(char), 1>;
    using WString = ArrayWithBucket<char32, 16/sizeof(char32), 1>;
#else
    using String = Array<char, 1>;
    using WString = Array<char32, 1>;
#endif

String operator+(const char *cString, String &&string);
String operator+(const char *cString, const String &string);
WString operator+(const char32 *cString, WString &&string);
WString operator+(const char32 *cString, const WString &string);

struct AlignText {
    u16 value;
    char fill;
    AlignText() = delete;
    inline AlignText(u16 alignment, char filler=' ') : value(alignment), fill(filler) {}
};

void AppendToString(String &string, u32 value, i32 base = 10);
void AppendToString(String &string, u64 value, i32 base = 10);
void AppendToString(String &string, u128 value, i32 base = 10);
void AppendToString(String &string, i32 value, i32 base = 10);
void AppendToString(String &string, i64 value, i32 base = 10);
void AppendToString(String &string, i128 value, i32 base = 10);
void AppendToString(String &string, f32 value, i32 base = 10, i32 precision = -1);
void AppendToString(String &string, f64 value, i32 base = 10, i32 precision = -1);
void AppendToString(String &string, f128 value, i32 base = 10, i32 precision = -1);

inline void AppendToString(String &string, u16 value, i32 base = 10) {
    AppendToString(string, (u32)value, base);
}
inline void AppendToString(String &string, i16 value, i32 base = 10) {
    AppendToString(string, (i32)value, base);
}

inline void AppendToString(String &string, AlignText alignment) {
    string.Resize(alignNonPowerOfTwo(string.size, alignment.value), alignment.fill);
}

template<typename T>
inline void AppendToString(String &string, T value) {
    string.Append(value);
}

template<typename T, typename... Args>
inline void AppendToString(String &string, T value, Args... args) {
    AppendToString(string, value);
    AppendToString(string, args...);
}

template<typename... Args>
inline String Stringify(Args... args) {
    String out;
    AppendToString(out, args...);
    return out;
}

template<typename T>
inline String ToString(T value) {
    String out;
    AppendToString(out, value);
    return out;
}

template<typename T>
inline String ToString(T value, i32 base) {
    String out;
    AppendToString(out, value, base);
    return out;
}

template<typename T>
inline String ToString(T value, i32 base, i32 precision) {
    String out;
    AppendToString(out, value, base, precision);
    return out;
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
