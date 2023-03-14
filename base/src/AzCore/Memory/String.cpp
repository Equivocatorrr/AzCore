/*
	File: String.cpp
	Author: Philip Haynes
*/

#include "String.hpp"
#include "../math.hpp"
#include "../memory.hpp"

AZCORE_STRING_TERMINATOR(char, '\0');
AZCORE_STRING_TERMINATOR(char32, 0u);

namespace AzCore {

String operator+(const char *cString, const String &string) {
	String value(string);
	return cString + std::move(value);
}

String operator+(const char *cString, String &&string) {
	String result;
	result.Reserve(StringLength(cString) + string.size);
	result.Append(cString);
	result.Append(std::move(string));
	return result;
}

WString operator+(const char32 *cString, const WString &string) {
	WString value(string);
	return cString + std::move(value);
}

WString operator+(const char32 *cString, WString &&string) {
	WString result;
	result.Reserve(StringLength(cString) + string.size);
	result.Append(cString);
	result.Append(std::move(string));
	return result;
}

template <typename T>
void Reverse(SimpleRange<T> range) {
	for (i32 i = 0, j = range.size-1; i < j; i++, j--) {
		Swap(range[i], range[j]);
	}
}

void AppendToStringWithBase(String &string, u32 value, i32 base) {
	if (value == 0) {
		string.Append("0");
		return;
	}
	i32 startSize = string.size;
	string.Reserve(startSize + i32(log((f32)value) / log((f32)base)) + 1);
	u32 remaining = value;
	while (remaining != 0) {
		u32 quot = remaining / base;
		u32 rem = remaining % base;
		if (base > 10) {
			string.Append(rem > 9 ? char(rem + 'a' - 10) : char(rem + '0'));
		} else {
			string.Append(char(rem + '0'));
		}
		remaining = quot;
	}
	Reverse(SimpleRange(string.data+startSize, string.size-startSize));
}

void AppendToStringWithBase(String &string, u64 value, i32 base) {
	if (value == 0) {
		string.Append("0");
		return;
	}
	i32 startSize = string.size;
	string.Reserve(startSize + i32((f32)log((f64)value) / log((f32)base)) + 1);
	u64 remaining = value;
	while (remaining != 0) {
		u64 quot = remaining / base;
		u64 rem = remaining % base;
		if (base > 10) {
			string.Append(rem > 9 ? char(rem + 'a' - 10) : char(rem + '0'));
		} else {
			string.Append(char(rem + '0'));
		}
		remaining = quot;
	}
	Reverse(SimpleRange(string.data+startSize, string.size-startSize));
}

#if AZCORE_COMPILER_SUPPORTS_128BIT_TYPES
void AppendToStringWithBase(String &string, u128 value, i32 base) {
	if (value == 0) {
		string.Append("0");
		return;
	}
	i32 startSize = string.size;
	string.Reserve(startSize + i32((f32)log((f64)value) / log((f32)base)) + 1);
	u128 remaining = value;
	while (remaining != 0) {
		u128 quot = remaining / base;
		u128 rem = remaining % base;
		if (base > 10) {
			string.Append(rem > 9 ? char(rem + 'a' - 10) : char(rem + '0'));
		} else {
			string.Append(char(rem + '0'));
		}
		remaining = quot;
	}
	Reverse(SimpleRange(string.data+startSize, string.size-startSize));
}
#endif

void AppendToStringWithBase(String &string, i32 value, i32 base) {
	if (value == 0) {
		string.Append("0");
		return;
	}
	i32 startSize = string.size;
	string.Reserve(startSize + i32(log((f32)value) / log((f32)base)) + 1);
	bool negative = value < 0;
	i32 remaining = abs(value);
	while (remaining != 0) {
		i32 quot = remaining / base;
		i32 rem = remaining % base;
		if (base > 10) {
			string.Append(rem > 9 ? char(rem + 'a' - 10) : char(rem + '0'));
		} else {
			string.Append(char(rem + '0'));
		}
		remaining = quot;
	}
	if (negative) {
		string += '-';
	}
	Reverse(SimpleRange(string.data+startSize, string.size-startSize));
}

void AppendToStringWithBase(String &string, i64 value, i32 base) {
	if (value == 0) {
		string.Append("0");
		return;
	}
	i32 startSize = string.size;
	string.Reserve(startSize + i32((f32)log((f64)value) / log((f32)base)) + 1);
	bool negative = value < 0;
	i64 remaining = abs(value);
	while (remaining != 0) {
		i64 quot = remaining / base;
		i64 rem = remaining % base;
		if (base > 10) {
			string.Append(rem > 9 ? char(rem + 'a' - 10) : char(rem + '0'));
		} else {
			string.Append(char(rem + '0'));
		}
		remaining = quot;
	}
	if (negative) {
		string += '-';
	}
	Reverse(SimpleRange(string.data+startSize, string.size-startSize));
}

#if AZCORE_COMPILER_SUPPORTS_128BIT_TYPES
void AppendToStringWithBase(String &string, i128 value, i32 base) {
	if (value == 0) {
		string.Append("0");
		return;
	}
	i32 startSize = string.size;
	string.Reserve(startSize + i32((f32)log((f64)value) / log((f32)base)) + 1);
	bool negative = value < 0;
	i128 remaining = abs(value);
	while (remaining != 0) {
		i128 quot = remaining / base;
		i128 rem = remaining % base;
		if (base > 10) {
			string.Append(rem > 9 ? char(rem + 'a' - 10) : char(rem + '0'));
		} else {
			string.Append(char(rem + '0'));
		}
		remaining = quot;
	}
	if (negative) {
		string += '-';
	}
	Reverse(SimpleRange(string.data+startSize, string.size-startSize));
}
#endif

char DigitToChar(i32 digit) {
	return digit >= 10 ? (digit + 'A' - 10) : (digit + '0');
}
char IncrementedDigit(char digit) {
	if (digit == '9') return 'A';
	return digit + 1;
}

// This is necessary for 128-bit floats because math.h pow doesn't support it.
// Might not be a bad idea anyway, but it hasn't been profiled.
// NOTE: This probably breaks perfect reproducibility because small
//       conversion errors accumulate into sizeable ones for some numbers.
template<typename Float>
Float intPow(i32 base, i32 exponent) {
	Float result = 1;
	while (exponent > 0) {
		result *= base;
		exponent--;
	}
	while (exponent < 0) {
		result /= base;
		exponent++;
	}
	return result;
}

// Specialize to use pow since that's more accurate.
template<>
f32 intPow(i32 base, i32 exponent) {
	return pow((f32)base, (f32)exponent);
}
template<>
f64 intPow(i32 base, i32 exponent) {
	return pow((f64)base, (f64)exponent);
}

template<typename Float, i32 MAX_SIGNIFICANT_DIGITS>
void _AppendFloatToString(String &string, Float value, i32 base, i32 precision) {
	const i32 MAX_SIGNIFICANT_DIGITS_BASED = ceil((f32)MAX_SIGNIFICANT_DIGITS/log2((f32)base));
	i32 startSize = string.size;
	string.Reserve(startSize + MAX_SIGNIFICANT_DIGITS_BASED + 4);
	i32 basisExponent = 0;
	Float remaining = value;
	if (remaining < 0.0f) {
		remaining = -remaining;
		string += '-';
	}
	i32 newExponent = 0;
	// Whether our string has the '.' in it already
	bool point = false;
	Float basis;
	
	// Find a basis that's the smallest power of base greater than the number
	if (remaining >= 1.0f) {
		while (true) {
			i32 newBasis = basisExponent+1;
			basis = intPow<Float>(base, newBasis);
			if (basis > remaining) {
				break;
			} else {
				newExponent++;
				basisExponent = newBasis;
			}
		}
	} else {
		while (true) {
			basisExponent--;
			newExponent--;
			basis = intPow<Float>(base, basisExponent);
			if (basis <= remaining) {
				break;
			}
		}
	}
	// The value near which we place our '.'
	Float crossover;
	i32 count = 1 + MAX_SIGNIFICANT_DIGITS_BASED;
	i32 dot = -1;
	constexpr i32 EXPONENT_LOW_BOUNDS = -3;
	const i32 EXPONENT_HIGH_BOUNDS = MAX_SIGNIFICANT_DIGITS_BASED;
	if (newExponent >= EXPONENT_HIGH_BOUNDS || newExponent <= EXPONENT_LOW_BOUNDS) {
		// For scientific notation, set our crossover to where the output will be around 1.0
		crossover = intPow<Float>(base, basisExponent-1);
	} else {
		// Regular decimal notation
		if (remaining < 1.0f) {
			string += "0.";
			dot = string.size-1;
			point = true;
			if (precision != -1)
				count = precision+1;
			for (i32 i = 2; i <= -newExponent; i++) {
				string += '0';
			}
		}
		// No special crossover
		crossover = 1.0f / base;
	}
	char lastDigit = base <= 10 ? '0'+base-1 : 'A'+base-11;
	// Whether we need to round up
	bool roundUp = false;
	basis = intPow<Float>(base, basisExponent);
	for (; count > 0; count--) {
		i32 digit = i32(remaining / basis);
		string += DigitToChar(digit);
		remaining -= basis * (Float)digit;
		if (remaining < 0.0f)
			remaining = 0.0f;
		basisExponent--;
		basis = intPow<Float>(base, basisExponent);
		if (point && count == 1) {
			if (i32(remaining / basis) >= intDivCeil(base, 2)) {
				roundUp = true;
			}
		}
		if (!point && basis <= crossover) {
			dot = string.size;
			string += '.';
			point = true;
			if (precision != -1)
				count = precision+1;
		}
	}
	// Do the actual rounding
	if (roundUp) {
		if (precision == -1) precision = string.size - dot - 1;
		string[dot+precision] = IncrementedDigit(string[dot+precision]);
		for (i32 i = dot+precision; i >= startSize;) {
			i32 nextI = i-1;
			if (nextI == dot) nextI--;
			if (string[i] > lastDigit) {
				if (i > dot+1) {
					string.Resize(i);
				} else {
					string[i] = '0';
				}
				if (nextI == startSize-1) {
					string.Insert(startSize, '1');
					dot++;
					break;
				}
				string[nextI] = IncrementedDigit(string[nextI]);
			} else {
				break;
			}
			i = nextI;
		}
	}
	i32 i = string.size - 1;
	for (; string[i] == '0'; i--) {
	}
	if (string[i] == '.') {
		i++; // Leave 1 trailing zero
	}
	string.Resize(i + 1);
	if (newExponent >= EXPONENT_HIGH_BOUNDS) {
		AppendToString(string, "e+");
		AppendToStringWithBase(string, newExponent, base);
	}
	else if (newExponent <= EXPONENT_LOW_BOUNDS) {
		AppendToString(string, "e-");
		AppendToStringWithBase(string, -newExponent, base);
	}
}

void AppendToStringWithBase(String &string, f32 value, i32 base, i32 precision) {
	u32 byteCode;
	memcpy((void *)&byteCode, (void *)&value, sizeof(byteCode));
	const bool negative = (byteCode & 0x80000000) != 0;
	u32 exponent = (byteCode >> 23) & 0xff;
	u32 significand = (byteCode & 0x007fffff) | (0x00800000); // Get our implicit bit in there.
	if (exponent == 0x00) {
		if (significand == 0x00800000) {
			string.Append(negative ? "-0.0" : "0.0");
			return;
		} else {
			significand &= 0x007fffff; // Get that implicit bit out of here!
		}
	}
	if (exponent == 0xff) {
		if (significand == 0x00800000) {
			string.Append(negative ? "-Infinity" : "Infinity");
		} else {
			string.Append(negative ? "-NaN" : "NaN");
		}
		return;
	}
	if (exponent == 150) {
		AppendToStringWithBase(string, negative ? -(i32)significand : (i32)significand, base);
		string.Append(".0");
		return;
	}
	_AppendFloatToString<f32, 24>(string, value, base, precision);
}

void AppendToStringWithBase(String &string, f64 value, i32 base, i32 precision) {
	u64 byteCode;
	memcpy((void *)&byteCode, (void *)&value, sizeof(byteCode));
	const bool negative = (byteCode & 0x8000000000000000) != 0;
	u32 exponent = (byteCode >> 52) & 0x7ff;
	u64 significand = (byteCode & 0x000fffffffffffff) | (0x0010000000000000); // Get our implicit bit in there.
	if (exponent == 0x0) {
		if (significand == 0x0010000000000000) {
			string.Append(negative ? "-0.0" : "0.0");
			return;
		} else {
			significand &= 0x000fffffffffffff; // Get that implicit bit out of here!
		}
	}
	if (exponent == 0x7ff) {
		if (significand == 0x0010000000000000) {
			string.Append(negative ? "-Infinity" : "Infinity");
		} else {
			string.Append(negative ? "-NaN" : "NaN");
		}
		return;
	}
	if (exponent == 1075) {
		AppendToStringWithBase(string, negative ? -(i64)significand : (i64)significand, base);
		string.Append(".0");
		return;
	}
	_AppendFloatToString<f64, 53>(string, value, base, precision);
}

#if AZCORE_COMPILER_SUPPORTS_128BIT_TYPES
void AppendToStringWithBase(String &string, f128 value, i32 base, i32 precision) {
	u128 byteCode;
	memcpy((void *)&byteCode, (void *)&value, sizeof(byteCode));
	const bool negative = (byteCode >> 127) != 0;
	i16 exponent = (byteCode >> 112) & 0x7fff;
	u128 significand = (byteCode << 16) >> 16 | ((u128)1 << 112); // Get our implicit bit in there.
	if (exponent == 0x0) {
		if (significand == (u128)1 << 112) {
			string.Append(negative ? "-0.0" : "0.0");
			return;
		} else {
			significand = (byteCode << 16) >> 16; // Get that implicit bit out of here!
		}
	}
	if (exponent == 0x7fff) {
		if (significand == (u128)1 << 112) {
			string.Append(negative ? "-Infinity" : "Infinity");
		} else {
			string.Append(negative ? "-NaN" : "NaN");
		}
		return;
	}
	exponent -= 16383;
	if (exponent == 112) {
		AppendToStringWithBase(string, (negative ? -(i128)significand : (i128)significand) >> (112 - exponent), base);
		string.Append(".0");
		return;
	}
	_AppendFloatToString<f128, 113>(string, value, base, precision);
}
#endif

#include <stdio.h>

template<typename Int, typename Char>
bool _StringToInt(SimpleRange<Char> string, Int *dst, i32 base) {
	Int multiplier = 1;
	Int result = 0;
	for (i32 i = string.size-1; i >= 0; i--) {
		i8 value = 0;
		Char c = string[i];
		if (isNumber(c)) {
			value = c - '0';
		} else if (c == '+') {
			*dst = result;
			return true;
		} else if (c == '-') {
			*dst = -result;
			return true;
		} else if (base > 10) {
			if (c >= 'a' && c < Char('a'+base-10)) {
				value = c - 'a' + 10;
			} else if (c >= 'A' && c < Char('A'+base-10)) {
				value = c - 'A' + 10;
			} else {
				return false;
			}
		} else {
			return false;
		}
		result += value * multiplier;
		multiplier *= base;
	}
	*dst = result;
	return true;
}

// TODO: Write some unit tests to confirm the reliability of this.
template<typename Float, typename Char>
bool _StringToFloat(StringBase<Char> string, Float *dst, i32 base) {
	Float out = Float(0);
	Float sign = Float(1);
	i32 exponent = 0;
	Float baseF = base;
	i32 dot = -1;
	i32 start;
	if (string[0] == '-') {
		sign = Float(-1);
		string.Erase(0);
	}
	for (i32 i = 0; i < string.size; i++) {
		if (string[i] == '.') {
			dot = i;
			string.Erase(i);
		}
	}
	// handle e-# and e+#
	for (i32 i = 0; i < string.size-2; i++) {
		if (string[i] == 'e' && (string[i+1] == '+' || string[i+1] == '-')) {
			i32 exp;
			if (!_StringToInt<i32, Char>(SimpleRange<Char>(&string[i+1], string.size-i-1), &exp, base)) return false;
			exponent = exp;
			/*
			if (exp > 0) {
				while (0 != exp--) {
					multiplier *= baseF;
				}
			} else if (exp < 0) {
				while (0 != exp++) {
					multiplier /= baseF;
				}
			}
			*/
			string.Resize(i);
			break;
		}
	}
	start = string.size - 1;
	if (dot == -1)
		dot = string.size;
	for (; dot <= start; dot++) {
		exponent--;
		// multiplier /= baseF;
	}
	for (i32 i = start; i >= 0; i--) {
		Float value = baseF;
		char c = string[i];
		if (isNumber(c)) {
			value = c - '0';
		} else if (base > 10) {
			if (c >= 'a' && c < 'a' + base-10) {
				value = c - 'a' + 10;
			} else if (c >= 'A' && c < 'A' + base-10) {
				value = c - 'A' + 10;
			} else {
				return false;
			}
		} else {
			return false;
		}
		AzAssert(value < baseF, "StringToFloat machine broke :(");
		out += value * intPow<Float>(base, exponent);
		exponent++;
		// multiplier *= baseF;
	}
	out *= sign;
	*dst = out;
	return true;
}

bool StringToF32(String string, f32 *dst, i32 base) {
	if (string == "Infinity") {
		*dst = INFINITY;
		return true;
	} else if (string == "-Infinity") {
		*dst = -INFINITY;
		return true;
	} else if (string == "NaN") {
		*dst = NAN;
		return true;
	} else if (string == "-NaN") {
		*dst = -NAN;
		return true;
	}
	return _StringToFloat<f32>(string, dst, base);
}

bool StringToF64(String string, f64 *dst, i32 base) {
	if (string == "Infinity") {
		*dst = INFINITY;
		return true;
	} else if (string == "-Infinity") {
		*dst = -INFINITY;
		return true;
	} else if (string == "NaN") {
		*dst = NAN;
		return true;
	} else if (string == "-NaN") {
		*dst = -NAN;
		return true;
	}
	return _StringToFloat<f64>(string, dst, base);
}

#if AZCORE_COMPILER_SUPPORTS_128BIT_TYPES
bool StringToF128(String string, f128 *dst, i32 base) {
	return _StringToFloat<f128>(string, dst, base);
}
#endif

static WString wInfinity = ToWString("Infinity");
static WString wNInfinity = ToWString("-Infinity");
static WString wNaN = ToWString("NaN");
static WString wNNaN = ToWString("-NaN");

bool WStringToF32(WString string, f32 *dst, i32 base) {
	if (string == wInfinity) {
		*dst = INFINITY;
		return true;
	} else if (string == wNInfinity) {
		*dst = -INFINITY;
		return true;
	} else if (string == wNaN) {
		*dst = NAN;
		return true;
	} else if (string == wNNaN) {
		*dst = -NAN;
		return true;
	}
	return _StringToFloat<f32>(string, dst, base);
}

bool StringToI32(String string, i32 *dst, i32 base) {
	return _StringToInt<i32, char>(string, dst, base);
}

bool StringToI64(String string, i64 *dst, i32 base) {
	return _StringToInt<i64, char>(string, dst, base);
}

#if AZCORE_COMPILER_SUPPORTS_128BIT_TYPES
bool StringToI128(String string, i128 *dst, i32 base) {
	return _StringToInt<i128, char>(string, dst, base);
}
#endif

bool equals(const char *a, const char *b) {
	for (u32 i = 0; a[i] != 0; i++) {
		if (a[i] != b[i])
			return false;
	}
	return true;
}

WString ToWString(String string) {
	return ToWString(string.data);
}

WString ToWString(const char *string) {
	WString out;
	for (u32 i = 0; string[i] != 0; i++) {
		char32 chr = string[i];
		if (!(chr & 0x80)) {
			chr &= 0x7F;
		}
		else if ((chr & 0xE0) == 0xC0) {
			chr &= 0x1F;
			chr <<= 6;
			chr += u32(string[++i] & 0x3F);
		}
		else if ((chr & 0xF0) == 0xE0) {
			chr &= 0xF;
			chr <<= 12;
			chr += u32(string[++i] & 0x3F) << 6;
			chr += u32(string[++i] & 0x3F);
		}
		else if ((chr & 0xF8) == 0xF0) {
			chr &= 0x7;
			chr <<= 18;
			chr += u32(string[++i] & 0x3F) << 12;
			chr += u32(string[++i] & 0x3F) << 6;
			chr += u32(string[++i] & 0x3F);
		}
		out += chr;
	}
	return out;
}

String FromWString(WString string) {
	String out;
	for (i32 i = 0; i < string.size; i++) {
		char32 chr = string[i];
		if (chr <= 0x7f) {
			out += (char)chr;
			continue;
		}
		if (chr <= 0x7ff) {
			out += 0b11000000 | (chr >> 6);
			out += 0b10000000 | (chr & 0b111111);
			continue;
		}
		if (chr <= 0xffff) {
			out += 0b11100000 | (chr >> 12);
			out += 0b10000000 | ((chr >> 6) & 0b111111);
			out += 0b10000000 | (chr & 0b111111);
			continue;
		}
		if (chr <= 0x10ffff) {
			out += 0b11110000 | (chr >> 18);
			out += 0b10000000 | ((chr >> 12) & 0b111111);
			out += 0b10000000 | ((chr >> 6) & 0b111111);
			out += 0b10000000 | (chr & 0b111111);
		}
	}
	return out;
}

i32 CharLen(const char chr) {
	if (!(chr & 0x80)) {
		return 1;
	}
	else if ((chr & 0xE0) == 0xC0) {
		return 2;
	}
	else if ((chr & 0xF0) == 0xE0) {
		return 3;
	}
	else if ((chr & 0xF8) == 0xF0) {
		return 4;
	}
	return 1;
}

void TrimWhitespace(String &string) {
	i32 leading = -1;
	i32 trailing = 0;
	for (i32 i = 0; i < string.size; i++) {
		trailing++;
		if (!isWhitespace(string[i])) {
			if (leading == -1) leading = i;
			trailing = 0;
		}
	}
	if (leading != -1) string.Erase(0, leading);
	string.Resize(string.size - trailing);
}

String Join(const Array<SimpleRange<char>> &values, SimpleRange<char> joiner) {
	String output;
	for (const SimpleRange<char> &value : values) {
		if (value.size) {
			output.Append(value);
			output.Append(joiner);
		}
	}
	if (output.size > joiner.size)
		output.size -= joiner.size;
	return output;
}

template<typename char_t>
Array<SimpleRange<char_t>> _SeparateByNewlines(SimpleRange<char_t> string, bool allowEmpty) {
	Array<SimpleRange<char_t>> result;
	i64 rangeStart = 0;
	for (i64 i = 0; i < string.size; i++) {
		char_t c = string[i];
		if (c == '\r' || c == '\n') {
			if (allowEmpty || i-rangeStart > 0) {
				result.Append(SimpleRange(&string[rangeStart], i-rangeStart));
			}
			if (c == '\r' && string.size > i+1 && string[i+1] == '\n') {
				++i;
			}
			rangeStart = i+1;
		}
	}
	if (rangeStart < string.size) {
		result.Append(SimpleRange(&string[rangeStart], string.size-rangeStart));
	}
	return result;
}

Array<Str> SeparateByNewlines(Str string, bool allowEmpty) {
	return _SeparateByNewlines<char>(string, allowEmpty);
}

Array<Str32> SeparateByNewlines(Str32 string, bool allowEmpty) {
	return _SeparateByNewlines<char32>(string, allowEmpty);
}

void StrToLower(Str str) {
	for (i32 i = 0; i < str.size; i++) {
		str[i] = CharToLower(str[i]);
	}
}

void StrToUpper(Str str) {
	for (i32 i = 0; i < str.size; i++) {
		str[i] = CharToUpper(str[i]);
	}
}

} // namespace AzCore
