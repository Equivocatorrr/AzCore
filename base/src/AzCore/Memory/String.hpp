/*
	File: String.hpp
	Author: Philip Haynes
*/

#ifndef AZCORE_STRING_HPP
#define AZCORE_STRING_HPP

#define AZCORE_STRING_WITH_BUCKET

#include "Util.hpp"
#include "Range.hpp"
#ifdef AZCORE_STRING_WITH_BUCKET
	#include "ArrayWithBucket.hpp"
#endif
#include "Array.hpp"

namespace AzCore {

// You can use Str in place of String as long as you're aware of the lifetime of the memory it points to since it has no storage of its own.
// Also useful for making a common interface for both const char literals and String lvalues.
using Str = SimpleRange<char>;
using Str32 = SimpleRange<char32>;

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

struct IndentState {
	String string;
	Array<i32> layers;
};

extern thread_local IndentState _indentState;

// Used to indent all future lines in this call to Stringify
struct Indent {
	char character;
	i32 count;
	inline Indent(char _character='\t', i32 _count=1) : character(_character), count(_count) {
		AzAssert(count >= 0, "Indent cannot be negative");
	}
};

// Used to undo the last Indent in this call to Stringify
struct IndentLess {};
// Used to clear all indenting in this call to Stringify
struct IndentClear {};

inline void AppendToString(String &string, Indent indent) {
	_indentState.layers.Append(_indentState.string.size);
	for (i32 i = 0; i < indent.count; i++) {
		_indentState.string.Append(indent.character);
	}
}

inline void AppendToString(String &string, IndentLess sup) {
	(void)sup;
	AzAssert(_indentState.layers.size > 0, "Cannot IndentLess, we're already at no indent!");
	_indentState.string.size = _indentState.layers.Back();
	_indentState.layers.size--;
}

inline void AppendToString(String &string, IndentClear yo) {
	(void)yo;
	_indentState.layers.ClearSoft();
	_indentState.string.ClearSoft();
}

inline void AppendToString(String &string, char value) {
	string.Append(value);
	if (value == '\n' && _indentState.string.size != 0) {
		string.Append(_indentState.string);
	}
}

inline void AppendToString(String &string, const char *value) {
	if (_indentState.string.size == 0) {
		string.Append(value);
	} else {
		for (i32 i = 0; value[i] != 0; i++) {
			string.Append(value[i]);
			if (value[i] == '\n') {
				string.Append(_indentState.string);
			}
		}
	}
}

inline void AppendToString(String &string, SimpleRange<char> value) {
	if (_indentState.string.size == 0) {
		string.Append(value);
	} else {
		for (i32 i = 0; i < value.size; i++) {
			string.Append(value[i]);
			if (value[i] == '\n') {
				string.Append(_indentState.string);
			}
		}
	}
}

inline void AppendToString(String &string, const String &value) {
	AppendToString(string, Str(value));
}

inline void AppendToString(String &string, Range<char> value) {
	AppendToString(string, SimpleRange(value));
}

inline void AppendToString(String &string, String &&value) {
	if (_indentState.string.size == 0) {
		string.Append(std::forward<String>(value));
	} else {
		for (i32 i = 0; i < value.size; i++) {
			string.Append(value[i]);
			if (value[i] == '\n') {
				string.Append(_indentState.string);
			}
		}
	}
}

template<typename... Args>
inline void AppendMultipleToString(String &string, Args&&... args) {
	static_assert(sizeof...(Args) > 0);
	(AppendToString(string, std::forward<Args>(args)), ...);
	_indentState.layers.ClearSoft();
	_indentState.string.ClearSoft();
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

bool StringToI32(Str string, i32 *dst, i32 base = 10);
bool StringToI64(Str string, i64 *dst, i32 base = 10);

#if AZCORE_COMPILER_SUPPORTS_128BIT_TYPES
bool StringToF128(String string, f128 *dst, i32 base = 10);
bool StringToI128(Str string, i128 *dst, i32 base = 10);
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

template <typename T>
String Join(const Array<T> &values, Str joiner) {
	String output;
	for (const T &value : values) {
		AppendToString(output, value);
		AppendToString(output, joiner);
	}
	if (output.size > joiner.size)
		output.size -= joiner.size;
	return output;
}

String Join(const Array<Str, 0> &values, Str joiner);

Array<Str, 0> SeparateByNewlines(Str string, bool allowEmpty=false);
Array<Str32, 0> SeparateByNewlines(Str32 string, bool allowEmpty=false);

void StrToLower(Str str);
void StrToUpper(Str str);

[[nodiscard]] Array<char> FileContents(String filename, bool binary=true);

template<typename T, i32 allocTail>
[[nodiscard]] Array<Range<T>, 0> SeparateByValues(Array<T, allocTail> &array, const ArrayWithBucket<T, 16/sizeof(T), allocTail> &values, bool allowEmpty=false) {
	Array<Range<T>, 0> result;
	i32 rangeStart = 0;
	for (i32 i = 0; i < array.size; i++) {
		if (values.Contains(array.data[i])) {
			if (allowEmpty || i-rangeStart > 0) {
				result.Append(array.GetRange(rangeStart, i-rangeStart));
			}
			rangeStart = i+1;
		}
	}
	if (rangeStart < array.size) {
		result.Append(array.GetRange(rangeStart, array.size-rangeStart));
	}
	return result;
}

template<typename T, i32 allocTail, i32 noAllocCount>
[[nodiscard]] Array<Range<T>, 0> SeparateByValues(ArrayWithBucket<T, noAllocCount, allocTail> &array,
		const ArrayWithBucket<T, 16/sizeof(T), allocTail> &values, bool allowEmpty=false) {
	Array<Range<T>, 0> result;
	i32 rangeStart = 0;
	for (i32 i = 0; i < array.size; i++) {
		if (values.Contains(array.data[i])) {
			if (allowEmpty || i-rangeStart > 0) {
				result.Append(array.GetRange(rangeStart, i-rangeStart));
			}
			rangeStart = i+1;
		}
	}
	if (rangeStart < array.size) {
		result.Append(array.GetRange(rangeStart, array.size-rangeStart));
	}
	return result;
}

template<typename T, i32 allocTail=0>
[[nodiscard]] Array<Range<T>, 0> SeparateByValues(Range<T> &range,
		const ArrayWithBucket<T, 16/sizeof(T), allocTail> &values, bool allowEmpty=false) {
	Array<Range<T>, 0> result;
	i32 rangeStart = 0;
	for (i32 i = 0; i < range.size; i++) {
		if (values.Contains(range[i])) {
			if (allowEmpty || i-rangeStart > 0) {
				result.Append(range.SubRange(rangeStart, i-rangeStart));
			}
			rangeStart = i+1;
		}
	}
	if (rangeStart < range.size) {
		result.Append(range.SubRange(rangeStart, range.size-rangeStart));
	}
	return result;
}

template<typename T, i32 allocTail=0>
[[nodiscard]] Array<Range<T>, 0> SeparateByValues(T *array,
		const ArrayWithBucket<T, 16/sizeof(T), allocTail> &values, bool allowEmpty=false) {
	Array<Range<T>, 0> result;
	i32 rangeStart = 0;
	for (i32 i = 0; array[i] != StringTerminators<T>::value; i++) {
		if (values.Contains(array[i])) {
			if (allowEmpty || i-rangeStart > 0) {
				result.Append(Range<T>(&array[rangeStart], i-rangeStart));
			}
			rangeStart = i+1;
		}
	}
	if (array[rangeStart] != StringTerminators<T>::value) {
		result.Append(Range<T>(&array[rangeStart], StringLength(array+rangeStart)));
	}
	return result;
}

template<typename T, i32 allocTail=0>
[[nodiscard]] Array<Range<T>, 0> SeparateByStrings(Array<T, allocTail> &array,
		const ArrayWithBucket<SimpleRange<T>, 16/sizeof(SimpleRange<T>), 0> &strings, bool allowEmpty=false) {
	Array<Range<T>, 0> result;
	i32 rangeStart = 0;
	for (i32 i = 0; i < array.size;) {
		i32 foundLen = 0;
		for (const SimpleRange<T> &r : strings) {
			i32 len = 0;
			while (len < r.size && i+len < array.size && r[len] == array[i+len]) {
				len++;
			}
			if (len == r.size && len > foundLen)
				foundLen = len;
		}
		if (foundLen > 0) {
			if (allowEmpty || i-rangeStart > 0) {
				result.Append(array.GetRange(rangeStart, i-rangeStart));
			}
			i += foundLen;
			rangeStart = i;
		} else {
			i++;
		}
	}
	if (rangeStart < array.size) {
		result.Append(array.GetRange(rangeStart, array.size-rangeStart));
	}
	return result;
}
// TODO: Implement SeparateByStrings for ArrayWithBucket, Range, and raw strings.

void _AssertFailure(const char *file, const char *line, const String &message);

} // namespace AzCore

#endif // AZCORE_STRING_HPP
