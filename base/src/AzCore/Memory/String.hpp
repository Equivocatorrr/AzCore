/*
	File: String.hpp
	Author: Philip Haynes
*/

#ifndef AZCORE_STRING_HPP
#define AZCORE_STRING_HPP

#define AZCORE_STRING_WITH_BUCKET

#include "../Utility/Memory.hpp"
#include "Range.hpp"
#ifdef AZCORE_STRING_WITH_BUCKET
	#include "ArrayWithBucket.hpp"
#endif
#include "Array.hpp"
#include "StringCommon.hpp"

#include <type_traits>

AZCORE_STRING_TERMINATOR(char, 0);
AZCORE_STRING_TERMINATOR(char32, 0);

namespace AzCore {

// You can use Str in place of String as long as you're aware of the lifetime of the memory it points to since it has no storage of its own.
// Also useful for making a common interface for both const char literals and String lvalues.
using Str = Range<char>;
using Str32 = Range<char32>;

// returns how many bytes in the string make up a single valid UTF-8 code point, or 0 if there is none.
constexpr i32 Utf8LengthSingle(Str str, i32 outputOnSingleOrInvalid=1) {
	if (str.size < 2) return outputOnSingleOrInvalid;
	constexpr u8 extMask = 0b1100'0000;
	constexpr u8 extVal  = 0b1000'0000;
	u8 c0 = str[0];
	u8 c1 = str[1];
	if ((c0 & 0b1110'0000) == 0b1100'0000) {
		// 2-byte candidate
		if ((c1 & extMask) == extVal) {
			return 2;
		}
	} else if ((c0 & 0b1111'0000) == 0b1110'0000) {
		// 3-byte candidate
		if (str.size >= 3) {
			u8 c2 = str[2];
			if ((c1 & extMask) == extVal && (c2 & extMask) == extVal) {
				return 3;
			}
		}
	} else if ((c0 & 0b1111'1000) == 0b1111'0000) {
		// 4-byte candidate
		if (str.size >= 4) {
			u8 c2 = str[2];
			u8 c3 = str[3];
			if ((c1 & extMask) == extVal && (c2 & extMask) == extVal && (c3 & extMask) == extVal) {
				return 4;
			}
		}
	}
	return outputOnSingleOrInvalid;
}

// returns how many total characters there are in a string, making sure to count multi-byte characters as single.
constexpr i32 Utf8CharCount(Str str, i32 *dstLastLineStart=nullptr) {
	i32 count = 0;
	if (dstLastLineStart) {
		*dstLastLineStart = 0;
	}
	for (i64 i = 0; i < str.size;) {
		count += 1;
		if (dstLastLineStart && str[i] == '\n') {
			*dstLastLineStart = count;
		}
		i += Utf8LengthSingle(str.SubRange(i));
	}
	return count;
}

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

// Because unsigned long is considered a distinct type from unsigned long long even if they're the same width
inline void AppendToStringWithBase(String &string, unsigned long value, i32 base) {
	AppendToStringWithBase(string, (u64)value, base);
}
inline void AppendToString(String &string, unsigned long value) {
	AppendToStringWithBase(string, value, 10);
}

// Because long is considered a distinct type from long long even if they're the same width
inline void AppendToStringWithBase(String &string, long value, i32 base) {
	AppendToStringWithBase(string, (i64)value, base);
}
inline void AppendToString(String &string, long value) {
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

extern thread_local i32 _preciseFloatToStringMode;

// If you need perfectly-reproducible float to string to float conversions, instantiate one of these.
struct PreciseFloatToStringMode {
	i32 diff=1;
	// Use in a Stringify call only!
	static inline PreciseFloatToStringMode On() {
		_preciseFloatToStringMode -= 1;
		return PreciseFloatToStringMode();
	}
	// Use in a Stringify call only!
	static inline PreciseFloatToStringMode Off() {
		_preciseFloatToStringMode += 1;
		return PreciseFloatToStringMode(-1);
	}
	inline PreciseFloatToStringMode() {
		_preciseFloatToStringMode += diff;
	}
	inline ~PreciseFloatToStringMode() {
		_preciseFloatToStringMode -= diff;
	}
private:
	inline PreciseFloatToStringMode(i32 _diff) : diff(_diff) {
		_preciseFloatToStringMode += diff;
	}
};

inline void AppendToString(String &string, const PreciseFloatToStringMode &mode) {
	_preciseFloatToStringMode += mode.diff;
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

// Not persistent, only aligns once per instance.
struct AlignText {
	enum ModeBits {
		// Counts UTF-8 characters instead of bytes and aligns to the start of each line instead of the whole string
		DEFAULT=0,
		// Will align to the start of the string, not each line.
		STRING_START = 0x01,
		// Will ignore UTF-8 encoding and just use bytes (efficient, but alignment will be wrong with multi-byte UTF-8 code points)
		ASCII = 0x02,
	};
	u32 value;
	u32 modeBits;
	Str fill;
	AlignText() = delete;
	inline AlignText(u32 alignment, Str filler=" ", u32 _modeBits=DEFAULT) : value(alignment), modeBits(_modeBits), fill(filler) {}
};

inline void AppendToString(String &string, AlignText alignment) {
	i32 oldSize, newSize;
	if (alignment.modeBits & AlignText::ASCII) {
		char fill = alignment.fill.size ? alignment.fill[0] : '\0';
		if (alignment.modeBits & AlignText::STRING_START) {
			oldSize = string.size;
			newSize = alignNonPowerOfTwo(oldSize, alignment.value);
		} else {
			i32 lastLineStart = 0;
			for (i32 i = string.size-1; i >= 0; i--) {
				if (string[i] == '\n') {
					lastLineStart = i+1;
					break;
				}
			}
			oldSize = string.size;
			newSize = lastLineStart + alignNonPowerOfTwo(oldSize-lastLineStart, alignment.value);
		}
		string.Resize(newSize, fill);
	} else {
		if (alignment.modeBits & AlignText::STRING_START) {
			oldSize = Utf8CharCount(string);
			newSize = alignNonPowerOfTwo(oldSize, alignment.value);
		} else {
			i32 lastLineStart;
			oldSize = Utf8CharCount(string, &lastLineStart);
			newSize = lastLineStart + alignNonPowerOfTwo(oldSize-lastLineStart, alignment.value);
		}
		for (i32 i = oldSize; i < newSize; i++) {
			string.Append(alignment.fill);
		}
	}
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

template<typename T>
inline void AppendToString(String &string, Range<T> array) {
	AppendToString(string, "{ ");
	for (i32 i = 0;;) {
		AppendToString(string, array[i]);
		if (++i >= array.size) break;
		AppendToString(string, ", ");
	}
	AppendToString(string, " }");
}

template<
	typename T,
	typename = std::enable_if_t<std::is_integral_v<T>>
>
inline void AppendToStringWithBase(String &string, Range<T> array, i32 base) {
	AppendToString(string, "{ ");
	for (i32 i = 0;;) {
		AppendToStringWithBase(string, array[i], base);
		if (++i >= array.size) break;
		AppendToString(string, ", ");
	}
	AppendToString(string, " }");
}

template<
	typename T,
	typename = std::enable_if_t<std::is_floating_point_v<T>>
>
inline void AppendToStringWithBase(String &string, Range<T> array, i32 base, i32 precision) {
	AppendToString(string, "{ ");
	for (i32 i = 0;;) {
		AppendToStringWithBase(string, array[i], base, precision);
		if (++i >= array.size) break;
		AppendToString(string, ", ");
	}
	AppendToString(string, " }");
}

template<typename T, i32 allocTail>
inline void AppendToString(String &string, const Array<T, allocTail> &array) {
	AppendToString(string, Range<T>(array));
}

template<typename T, i32 noAllocCount, i32 allocTail>
inline void AppendToString(String &string, const ArrayWithBucket<T, noAllocCount, allocTail> &array) {
	AppendToString(string, Range<T>(array));
}

inline void AppendToString(String &string, Range<char> value) {
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

inline void AppendToString(String &string, SmartRange<char> value) {
	AppendToString(string, Str(value));
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

template<typename T>
struct FormatFloat {
	T value;
	i32 _base;
	i32 _precision;
	FormatFloat() = delete;
	inline FormatFloat(T in, i32 base, i32 precision=-1) : value(in), _base(base), _precision(precision) {}
};

template<typename T>
force_inline(void) AppendToString(String &string, FormatFloat<T> fmt) {
	AppendToStringWithBase(string, fmt.value, fmt._base, fmt._precision);
}

template<typename T>
struct FormatInt {
	T value;
	i32 _base;
	bool _addBasePrefix;
	FormatInt() = delete;
	inline FormatInt(T in, i32 base, bool addBasePrefix=false) : value(in), _base(base), _addBasePrefix(addBasePrefix) {}
};

template<typename T>
force_inline(void) AppendToString(String &string, FormatInt<T> fmt) {
	if (fmt._addBasePrefix) {
		switch (fmt._base) {
			case 2:
				AppendToString(string, "0b");
				break;
			case 8:
				AppendToString(string, "0o");
				break;
			case 10:
				break;
			case 16:
				AppendToString(string, "0x");
				break;
			default: // Not sure what else to do tbh
				AppendToString(string, "0_");
				AppendToString(string, fmt._base);
				AppendToString(string, '_');
				break;
		}
	}
	AppendToStringWithBase(string, fmt.value, fmt._base);
}

struct EscapeString {
	Str value;
	char _quote;
	bool _utf8;
	EscapeString() = delete;
	explicit inline EscapeString(Str in, char quote='"', bool utf8=true) : value(in), _quote(quote), _utf8(utf8) {}
};

constexpr Str _low32Escapes[32] = {
	"\\0"  , "\\001", "\\002", "\\003", "\\004", "\\005", "\\006", "\\a"  ,
	"\\b"  , "\\t"  , "\\n"  , "\\v"  , "\\f"  , "\\r"  , "\\016", "\\017",
	"\\020", "\\021", "\\022", "\\023", "\\024", "\\025", "\\026", "\\027",
	"\\030", "\\031", "\\032", "\\e"  , "\\034", "\\035", "\\036", "\\037",
};

force_inline(void) AppendToString(String &string, EscapeString value) {
	i32 utf8ToGo = 0;
	for (i32 i = 0; i < value.value.size; i++) {
		char c = value.value[i];
		if (c >= 0 && c < 32) {
			AppendToString(string, _low32Escapes[(u32)c]);
			continue;
		} else if (c < 0) {
			if (value._utf8 && utf8ToGo == 0) {
				utf8ToGo = Utf8LengthSingle(value.value.SubRange(i), 0);
			}
			if (utf8ToGo == 0) {
				// Not a valid UTF-8 code sequence (or we only allow ascii)
				string.Append('\\');
				// Don't worry about leading zeroes since we can only be here vith values >= 0200
				AppendToStringWithBase(string, (u32)(u8)c, 8);
				continue;
			} else {
				utf8ToGo--;
			}
		} else if (c == value._quote) {
			string.Append('\\');
			string.Append(value._quote);
			continue;
		} else if (c == '\\') {
			AppendToString(string, "\\\\");
			continue;
		} else if (c == '\177') {
			AppendToString(string, "\\177");
			continue;
		}
		string.Append(c);
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

[[nodiscard]] Array<char> FileContents(String filepath, bool binary=true);

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
[[nodiscard]] Array<SmartRange<T>, 0> SeparateByValues(SmartRange<T> &range,
		const ArrayWithBucket<T, 16/sizeof(T), allocTail> &values, bool allowEmpty=false) {
	Array<SmartRange<T>, 0> result;
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
		const ArrayWithBucket<Range<T>, 16/sizeof(Range<T>), 0> &strings, bool allowEmpty=false) {
	Array<Range<T>, 0> result;
	i32 rangeStart = 0;
	for (i32 i = 0; i < array.size;) {
		i32 foundLen = 0;
		for (const Range<T> &r : strings) {
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
