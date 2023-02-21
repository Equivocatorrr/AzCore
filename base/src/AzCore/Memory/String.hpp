/*
	File: String.hpp
	Author: Philip Haynes
*/

#ifndef AZCORE_STRING_HPP
#define AZCORE_STRING_HPP

#include "ArrayWithBucket.hpp"
#include "Range.hpp"

namespace AzCore {

size_t align(size_t size, size_t alignment);
size_t alignNonPowerOfTwo(size_t size, size_t alignment);

#define AZCORE_STRING_WITH_BUCKET
#ifdef AZCORE_STRING_WITH_BUCKET
	template<typename T>
	using StringBase = ArrayWithBucket<T, 16/sizeof(T), 1>;
#else
	template<typename T>
	using StringBase = Array<T, 1>;
#endif
using String = StringBase<char>;
using WString = StringBase<char32>;

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

template<typename T>
struct FormatFloat {
	T value;
	i32 _base;
	i32 _precision;
	FormatFloat() = delete;
	inline FormatFloat(T in, i32 base, i32 precision=-1) : value(in), _base(base), _precision(precision) {}
};

template<typename T>
struct FormatInt {
	T value;
	i32 _base;
	FormatInt() = delete;
	inline FormatInt(T in, i32 base) : value(in), _base(base) {}
};

void AppendToStringWithBase(String &string, u32 value, i32 base);
void AppendToStringWithBase(String &string, u64 value, i32 base);
void AppendToStringWithBase(String &string, i32 value, i32 base);
void AppendToStringWithBase(String &string, i64 value, i32 base);
void AppendToStringWithBase(String &string, f32 value, i32 base, i32 precision = -1);
void AppendToStringWithBase(String &string, f64 value, i32 base, i32 precision = -1);

inline void AppendToString(String &string, u32 value) {
	AppendToStringWithBase(string, value, 10);
}
inline void AppendToString(String &string, u64 value) {
	AppendToStringWithBase(string, value, 10);
}
inline void AppendToString(String &string, i32 value) {
	AppendToStringWithBase(string, value, 10);
}
inline void AppendToString(String &string, i64 value) {
	AppendToStringWithBase(string, value, 10);
}
inline void AppendToString(String &string, f32 value) {
	AppendToStringWithBase(string, value, 10);
}
inline void AppendToString(String &string, f64 value) {
	AppendToStringWithBase(string, value, 10);
}

#if AZCORE_COMPILER_SUPPORTS_128BIT_TYPES
void AppendToStringWithBase(String &string, u128 value, i32 base);
void AppendToStringWithBase(String &string, i128 value, i32 base);
void AppendToStringWithBase(String &string, f128 value, i32 base, i32 precision = -1);

inline void AppendToString(String &string, u128 value) {
	AppendToStringWithBase(string, value, 10);
}
inline void AppendToString(String &string, i128 value) {
	AppendToStringWithBase(string, value, 10);
}
inline void AppendToString(String &string, f128 value) {
	AppendToStringWithBase(string, value, 10);
}
#endif

template<typename T>
force_inline(void) AppendToString(String &string, FormatFloat<T> fmt) {
	AppendToStringWithBase(string, fmt.value, fmt._base, fmt._precision);
}

template<typename T>
force_inline(void) AppendToString(String &string, FormatInt<T> fmt) {
	AppendToStringWithBase(string, fmt.value, fmt._base);
}

inline void AppendToStringWithBase(String &string, u16 value, i32 base) {
	AppendToStringWithBase(string, (u32)value, base);
}
inline void AppendToStringWithBase(String &string, i16 value, i32 base) {
	AppendToStringWithBase(string, (i32)value, base);
}
inline void AppendToString(String &string, u16 value) {
	AppendToString(string, (u32)value);
}
inline void AppendToString(String &string, i16 value) {
	AppendToString(string, (i32)value);
}

inline void AppendToString(String &string, AlignText alignment) {
	string.Resize(alignNonPowerOfTwo(string.size, alignment.value), alignment.fill);
}

inline void AppendToString(String &string, char value) {
	string.Append(value);
}

inline void AppendToString(String &string, const char *value) {
	string.Append(value);
}

inline void AppendToString(String &string, SimpleRange<char> value) {
	string.Append(value);
}

inline void AppendToString(String &string, Range<char> value) {
	string.Append(value);
}

inline void AppendToString(String &string, String &&value) {
	string.Append(std::forward<String>(value));
}

template<typename... Args>
inline void AppendMultipleToString(String &string, Args&&... args) {
	static_assert(sizeof...(Args) > 0);
	(AppendToString(string, std::forward<Args>(args)), ...);
}

template<typename... Args>
inline String Stringify(Args&&... args) {
	String out;
	AppendMultipleToString(out, std::forward<Args>(args)...);
	return out;
}

template<typename T>
inline String ToString(T &&value) {
	String out;
	AppendToString(out, std::forward<T>(value));
	return out;
}

template<typename T>
inline String ToString(T &&value, i32 base) {
	String out;
	AppendToStringWithBase(out, std::forward<T>(value), base);
	return out;
}

template<typename T>
inline String ToString(T &&value, i32 base, i32 precision) {
	String out;
	AppendToStringWithBase(out, std::forward<T>(value), base, precision);
	return out;
}

bool StringToF32(String string, f32 *dst, i32 base = 10);
bool StringToF64(String string, f64 *dst, i32 base = 10);

bool WStringToF32(WString string, f32 *dst, i32 base = 10);

bool StringToI32(String string, i32 *dst, i32 base = 10);
bool StringToI64(String string, i64 *dst, i32 base = 10);

#if AZCORE_COMPILER_SUPPORTS_128BIT_TYPES
bool StringToF128(String string, f128 *dst, i32 base = 10);
bool StringToI128(String string, i128 *dst, i32 base = 10);
#endif

inline bool operator==(const char *b, const String &a)
{
	return a == b;
}

bool equals(const char *a, const char *b);

// Converts a UTF-8 string to Unicode string
WString ToWString(const char *string);
// Converts a UTF-8 string to Unicode string
WString ToWString(String string);
// Converts a Unicode string to UTF-8
String FromWString(WString string);
// Returns how many bytes long a single UTF-8 character is based on the first.
i32 CharLen(const char chr);
inline char CharToUpper(char c) {
	if (c >= 'a' && c <= 'z') c = c + 'A' - 'a';
	return c;
}
inline char CharToLower(char c) {
	if (c >= 'A' && c <= 'Z') c = c + 'a' - 'A';
	return c;
}
inline bool isNewline(i32 c) {
    return c == '\n' || c == '\r';
}
inline bool isWhitespace(i32 c) {
    return c == ' ' || c == '\t' || isNewline(c);
}
inline bool isLowercase(i32 c) {
    return c >= 'a' && c <= 'z';
}
inline bool isUppercase(i32 c) {
    return c >= 'A' && c <= 'Z';
}
inline bool isText(i32 c) {
    return isLowercase(c) || isUppercase(c);
}
inline bool isNumber(i32 c) {
    return c >= '0' && c <= '9';
}
inline bool isWordChar(i32 c) {
    return c == '_'
        || isText(c)
        || isNumber(c);
}

inline bool isAlphaNumeric(i32 c) {
    return isNumber(c) || isText(c);
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

// Removes leading and trailing whitespace
void TrimWhitespace(String &string);

template<u16 bounds>
constexpr i32 IndexHash(const String &in) {
	u32 hash = 0;
	for (char c : in) {
		hash = hash * 31 + c;
	}
	return i32(hash % bounds);
}

String Join(const Array<SimpleRange<char>, 0> &values, SimpleRange<char> joiner);

Array<SimpleRange<char>, 0> SeparateByNewlines(SimpleRange<char> string, bool allowEmpty=false);
Array<SimpleRange<char32>, 0> SeparateByNewlines(SimpleRange<char32> string, bool allowEmpty=false);

} // namespace AzCore

#ifndef NDEBUG
inline void _Assert(bool condition, const char *file, const char *line, az::String message) {
	if (!condition) {
		fprintf(stderr, "\033[96m%s\033[0m:\033[96m%s\033[0m Assert failed: \033[91m%s\033[0m\n", file, line, message.data);
		PrintBacktrace(stderr);
		exit(1);
	}
}
#endif // NDEBUG

#endif // AZCORE_STRING_HPP
