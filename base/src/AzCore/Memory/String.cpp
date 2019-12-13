/*
    File: String.cpp
    Author: Philip Haynes
*/

#include "String.hpp"
#include "../math.hpp"

AZCORE_STRING_TERMINATOR(char, '\0');
AZCORE_STRING_TERMINATOR(char32, 0u);

namespace AzCore {

String operator+(const char *cString, const String &string)
{
    String value(string);
    return cString + std::move(value);
}

String operator+(const char *cString, String &&string)
{
    String result;
    result.Reserve(StringLength(cString) + string.size);
    result.Append(cString);
    result.Append(std::move(string));
    return result;
}

WString operator+(const char32 *cString, const WString &string)
{
    WString value(string);
    return cString + std::move(value);
}

WString operator+(const char32 *cString, WString &&string)
{
    WString result;
    result.Reserve(StringLength(cString) + string.size);
    result.Append(cString);
    result.Append(std::move(string));
    return result;
}

String ToString(const u32 &value, i32 base)
{
    if (value == 0)
    {
        return "0";
    }
    String out;
    out.Reserve(i32(log((f32)value) / log((f32)base)) + 1);
    u32 remaining = value;
    while (remaining != 0)
    {
        u32 quot = remaining / base;
        u32 rem = remaining % base;
        if (base > 10)
        {
            out.Append(rem > 9 ? char(rem + 'a' - 10) : char(rem + '0'));
        }
        else
        {
            out.Append(char(rem + '0'));
        }
        remaining = quot;
    }
    return out.Reverse();
}

String ToString(const u64 &value, i32 base)
{
    if (value == 0)
    {
        return "0";
    }
    String out;
    out.Reserve(i32((f32)log((f64)value) / log((f32)base)) + 1);
    u64 remaining = value;
    while (remaining != 0)
    {
        u64 quot = remaining / base;
        u64 rem = remaining % base;
        if (base > 10)
        {
            out.Append(rem > 9 ? char(rem + 'a' - 10) : char(rem + '0'));
        }
        else
        {
            out.Append(char(rem + '0'));
        }
        remaining = quot;
    }
    return out.Reverse();
}

String ToString(const u128 &value, i32 base)
{
    if (value == 0)
    {
        return "0";
    }
    String out;
    out.Reserve(i32((f32)log((f64)value) / log((f32)base)) + 1);
    u128 remaining = value;
    while (remaining != 0)
    {
        u128 quot = remaining / base;
        u128 rem = remaining % base;
        if (base > 10)
        {
            out.Append(rem > 9 ? char(rem + 'a' - 10) : char(rem + '0'));
        }
        else
        {
            out.Append(char(rem + '0'));
        }
        remaining = quot;
    }
    return out.Reverse();
}

String ToString(const i32 &value, i32 base)
{
    if (value == 0)
    {
        return "0";
    }
    String out;
    out.Reserve(i32(log((f32)value) / log((f32)base)) + 1);
    bool negative = value < 0;
    i32 remaining = abs(value);
    while (remaining != 0)
    {
        i32 quot = remaining / base;
        i32 rem = remaining % base;
        if (base > 10)
        {
            out.Append(rem > 9 ? char(rem + 'a' - 10) : char(rem + '0'));
        }
        else
        {
            out.Append(char(rem + '0'));
        }
        remaining = quot;
    }
    if (negative)
    {
        out += '-';
    }
    return out.Reverse();
}

String ToString(const i64 &value, i32 base)
{
    if (value == 0)
    {
        return "0";
    }
    String out;
    out.Reserve(i32((f32)log((f64)value) / log((f32)base)) + 1);
    bool negative = value < 0;
    i64 remaining = abs(value);
    while (remaining != 0)
    {
        i64 quot = remaining / base;
        i64 rem = remaining % base;
        if (base > 10)
        {
            out.Append(rem > 9 ? char(rem + 'a' - 10) : char(rem + '0'));
        }
        else
        {
            out.Append(char(rem + '0'));
        }
        remaining = quot;
    }
    if (negative)
    {
        out += '-';
    }
    return out.Reverse();
}

String ToString(const i128 &value, i32 base)
{
    if (value == 0)
    {
        return "0";
    }
    String out;
    out.Reserve(i32((f32)log((f64)value) / log((f32)base)) + 1);
    bool negative = value < 0;
    i128 remaining = abs(value);
    while (remaining != 0)
    {
        i128 quot = remaining / base;
        i128 rem = remaining % base;
        if (base > 10)
        {
            out.Append(rem > 9 ? char(rem + 'a' - 10) : char(rem + '0'));
        }
        else
        {
            out.Append(char(rem + '0'));
        }
        remaining = quot;
    }
    if (negative)
    {
        out += '-';
    }
    return out.Reverse();
}

String ToString(const f32 &value, i32 base, i32 precision)
{
    u32 byteCode;
    memcpy((void *)&byteCode, (void *)&value, sizeof(byteCode));
    const bool negative = (byteCode & 0x80000000) != 0;
    u32 exponent = (byteCode >> 23) & 0xff;
    u32 significand = (byteCode & 0x007fffff) | (0x00800000); // Get our implicit bit in there.
    if (exponent == 0x00)
    {
        if (significand == 0x00800000)
        {
            return negative ? "-0.0" : "0.0";
        }
        else
        {
            significand &= 0x007fffff; // Get that implicit bit out of here!
        }
    }
    if (exponent == 0xff)
    {
        if (significand == 0x00800000)
        {
            return negative ? "-Infinity" : "Infinity";
        }
        else
        {
            return negative ? "-NaN" : "NaN";
        }
    }
    if (exponent == 150)
    {
        return ToString(negative ? -(i32)significand : (i32)significand, base) + ".0";
    }
    String out;
    out.Reserve(12);
    f32 basis = 1.0;
    f32 remaining = value;
    if (remaining < 0.0)
    {
        remaining = -remaining;
        out += '-';
    }
    i32 newExponent = 0;
    bool point = false;
    if (remaining >= 1.0)
    {
        while (true)
        {
            f32 newBasis = basis * base;
            if (newBasis > remaining)
            {
                break;
            }
            else
            {
                newExponent++;
                basis = newBasis;
            }
        }
    }
    else
    {
        while (true)
        {
            basis /= base;
            newExponent--;
            if (basis <= remaining)
            {
                break;
            }
        }
    }
    f32 crossover;
    i32 count = 8;
    if (newExponent > 7 || newExponent < -1)
    {
        crossover = basis / base;
    }
    else
    {
        if (remaining < 1.0)
        {
            out += "0.";
            point = true;
            if (precision != -1)
                count = precision + 1;
            for (i32 i = 2; i < newExponent; i++)
            {
                out += '0';
            }
        }
        crossover = 1.0 / base;
    }
    for (; count > 0; count--)
    {
        i32 digit = remaining / basis;
        out += digit >= 10 ? (digit + 'A' - 10) : (digit + '0');
        remaining -= basis * (f32)digit;
        if (remaining < 0.0)
            remaining = 0.0;
        basis /= base;
        if (!point && basis <= crossover)
        {
            out += '.';
            point = true;
            if (precision != -1)
                count = precision + 1;
        }
    }
    i32 i = out.size - 1;
    for (; out[i] == '0'; i--)
    {
    }
    if (out[i] == '.')
    {
        i++; // Leave 1 trailing zero
    }
    out.Resize(i + 1);
    if (newExponent > 7)
    {
        out += "e+" + ToString(newExponent);
    }
    else if (newExponent < -1)
    {
        out += "e-" + ToString(-newExponent);
    }

    return out;
}

String ToString(const f64 &value, i32 base, i32 precision)
{
    u64 byteCode;
    memcpy((void *)&byteCode, (void *)&value, sizeof(byteCode));
    const bool negative = (byteCode & 0x8000000000000000) != 0;
    u32 exponent = (byteCode >> 52) & 0x7ff;
    u64 significand = (byteCode & 0x000fffffffffffff) | (0x0010000000000000); // Get our implicit bit in there.
    if (exponent == 0x0)
    {
        if (significand == 0x0010000000000000)
        {
            return negative ? "-0.0" : "0.0";
        }
        else
        {
            significand &= 0x000fffffffffffff; // Get that implicit bit out of here!
        }
    }
    if (exponent == 0x7ff)
    {
        if (significand == 0x0010000000000000)
        {
            return negative ? "-Infinity" : "Infinity";
        }
        else
        {
            return negative ? "-NaN" : "NaN";
        }
    }
    if (exponent == 1075)
    {
        return ToString(negative ? -(i64)significand : (i64)significand, base) + ".0";
    }
    String out;
    out.Reserve(24);
    f64 basis = 1.0;
    f64 remaining = value;
    if (remaining < 0.0)
    {
        remaining = -remaining;
        out += '-';
    }
    i32 newExponent = 0;
    bool point = false;
    if (remaining >= 1.0)
    {
        while (true)
        {
            f64 newBasis = basis * base;
            if (newBasis > remaining)
            {
                break;
            }
            else
            {
                newExponent++;
                basis = newBasis;
            }
        }
    }
    else
    {
        while (true)
        {
            basis /= base;
            newExponent--;
            if (basis <= remaining)
            {
                break;
            }
        }
    }
    f64 crossover;
    i32 count = 16;
    if (newExponent > 15 || newExponent < -1)
    {
        crossover = basis / base;
    }
    else
    {
        if (remaining < 1.0)
        {
            out += "0.";
            point = true;
            if (precision != -1)
                count = precision + 1;
            for (i32 i = 2; i < newExponent; i++)
            {
                out += '0';
            }
        }
        crossover = 1.0 / base;
    }
    for (; count > 0; count--)
    {
        i32 digit = remaining / basis;
        out += digit >= 10 ? (digit + 'A' - 10) : (digit + '0');
        remaining -= basis * (f64)digit;
        if (remaining < 0.0)
            remaining = 0.0;
        basis /= base;
        if (!point && basis <= crossover)
        {
            out += '.';
            point = true;
            if (precision != -1)
                count = precision + 1;
        }
    }
    i32 i = out.size - 1;
    for (; out[i] == '0'; i--)
    {
    }
    if (out[i] == '.')
    {
        i++; // Leave 1 trailing zero
    }
    out.Resize(i + 1);
    if (newExponent > 15)
    {
        out += "e+" + ToString(newExponent);
    }
    else if (newExponent < -1)
    {
        out += "e-" + ToString(-newExponent);
    }

    return out;
}

String ToString(const f128 &value, i32 base, i32 precision)
{
    u128 byteCode;
    memcpy((void *)&byteCode, (void *)&value, sizeof(byteCode));
    const bool negative = (byteCode >> 127) != 0;
    i16 exponent = (byteCode >> 112) & 0x7fff;
    u128 significand = (byteCode << 16) >> 16 | ((u128)1 << 112); // Get our implicit bit in there.
    if (exponent == 0x0)
    {
        if (significand == (u128)1 << 112)
        {
            return negative ? "-0.0" : "0.0";
        }
        else
        {
            significand = (byteCode << 16) >> 16; // Get that implicit bit out of here!
        }
    }
    if (exponent == 0x7fff)
    {
        if (significand == (u128)1 << 112)
        {
            return negative ? "-Infinity" : "Infinity";
        }
        else
        {
            return negative ? "-NaN" : "NaN";
        }
    }
    exponent -= 16383;
    if (exponent == 112)
    {
        return ToString((negative ? -(i128)significand : (i128)significand) >> (112 - exponent), base) + ".0";
    }
    String out;
    out.Reserve(40);

    f128 basis = 1.0;
    f128 remaining = value;
    if (remaining < 0.0)
    {
        remaining = -remaining;
        out += '-';
    }
    i32 newExponent = 0;
    bool point = false;
    if (remaining >= 1.0)
    {
        while (true)
        {
            f128 newBasis = basis * base;
            if (newBasis > remaining)
            {
                break;
            }
            else
            {
                newExponent++;
                basis = newBasis;
            }
        }
    }
    else
    {
        while (true)
        {
            basis /= base;
            newExponent--;
            if (basis <= remaining)
            {
                break;
            }
        }
    }
    f128 crossover;
    i32 count = 34;
    if (newExponent > 33 || newExponent < -1)
    {
        crossover = basis / base;
    }
    else
    {
        if (remaining < 1.0)
        {
            out += "0.";
            point = true;
            if (precision != -1)
                count = precision + 1;
            for (i32 i = 2; i < newExponent; i++)
            {
                out += '0';
            }
        }
        crossover = 1.0 / base;
    }
    for (; count > 0; count--)
    {
        i32 digit = remaining / basis;
        out += digit >= 10 ? (digit + 'A' - 10) : (digit + '0');
        remaining -= basis * (f128)digit;
        if (remaining < 0.0)
            remaining = 0.0;
        basis /= base;
        if (!point && basis <= crossover)
        {
            out += '.';
            point = true;
            if (precision != -1)
                count = precision + 1;
        }
    }
    i32 i = out.size - 1;
    for (; out[i] == '0'; i--)
    {
    }
    if (out[i] == '.')
    {
        i++; // Leave 1 trailing zero
    }
    out.Resize(i + 1);
    if (newExponent > 33)
    {
        out += "e+" + ToString(newExponent);
    }
    else if (newExponent < -1)
    {
        out += "e-" + ToString(-newExponent);
    }

    return out;
}

inline bool isNumber(char c)
{
    return c >= '0' && c <= '9';
}

// TODO: Write some unit tests to confirm the reliability of this.
f32 StringToF32(String string, i32 base)
{
    f64 out = 0.0;
    f64 multiplier = 1.0;
    f64 baseF = base;
    i32 dot = -1;
    i32 start;
    if (string[0] == '-')
    {
        multiplier = -1.0;
        string.Erase(0);
    }
    for (i32 i = 0; i < string.size; i++)
    {
        if (string[i] == '.')
        {
            dot = i;
            string.Erase(i);
        }
    }
    start = string.size - 1;
    if (dot == -1)
        dot = string.size;
    for (; dot <= start; dot++)
    {
        multiplier /= baseF;
    }
    for (i32 i = start; i >= 0; i--)
    {
        f64 value = baseF;
        char c = string[i];
        if (isNumber(c))
        {
            value = c - '0';
        }
        else if (base > 10)
        {
            if (c >= 'a' && c < 'a' + base-10)
            {
                value = c - 'a' + 10;
            }
            else if (c >= 'A' && c < 'A' + base-10)
            {
                value = c - 'A' + 10;
            }
        }
        if (value >= baseF)
        {
            return 0.0;
        }
        out += value * multiplier;
        multiplier *= baseF;
    }
    return out;
}

// This is literally the exact same code as above...
f32 WStringToF32(WString string, i32 base)
{
    f64 out = 0.0;
    f64 multiplier = 1.0;
    f64 baseF = base;
    i32 dot = -1;
    i32 start;
    if (string[0] == '-')
    {
        multiplier = -1.0;
        string.Erase(0);
    }
    for (i32 i = 0; i < string.size; i++)
    {
        if (string[i] == '.')
        {
            dot = i;
            string.Erase(i);
        }
    }
    start = string.size - 1;
    if (dot == -1)
        dot = string.size;
    for (; dot <= start; dot++)
    {
        multiplier /= baseF;
    }
    for (i32 i = start; i >= 0; i--)
    {
        f64 value = baseF;
        char c = string[i];
        if (isNumber(c))
        {
            value = c - '0';
        }
        else if (base > 10)
        {
            if (c >= 'a' && c < 'a' + base-10)
            {
                value = c - 'a' + 10;
            }
            else if (c >= 'A' && c < 'A' + base-10)
            {
                value = c - 'A' + 10;
            }
        }
        if (value >= baseF)
        {
            return 0.0;
        }
        out += value * multiplier;
        multiplier *= baseF;
    }
    return out;
}

i64 StringToI64(String string, i32 base) {
    i64 multiplier = 1;
    i64 result = 0;
    for (i32 i = string.size-1; i >= 0; i--) {
        i8 value = 0;
        char c = string[i];
        if (isNumber(c)) {
            value = c - '0';
        } else if (c == '-') {
            return -result;
        } else if (base > 10) {
            if (c >= 'a' && c < 'a'+base-10) {
                value = c - 'a' + 10;
            } else if (c >= 'A' && c < 'A'+base-10) {
                value = c - 'A' + 10;
            }
        }
        result += value * multiplier;
        multiplier *= base;
    }
    return result;
}

bool equals(const char *a, const char *b)
{
    for (u32 i = 0; a[i] != 0; i++)
    {
        if (a[i] != b[i])
            return false;
    }
    return true;
}

WString ToWString(String string)
{
    return ToWString(string.data);
}

WString ToWString(const char *string)
{
    WString out;
    for (u32 i = 0; string[i] != 0; i++)
    {
        char32 chr = string[i];
        if (!(chr & 0x80))
        {
            chr &= 0x7F;
        }
        else if ((chr & 0xE0) == 0xC0)
        {
            chr &= 0x1F;
            chr <<= 6;
            chr += u32(string[++i] & 0x3F);
        }
        else if ((chr & 0xF0) == 0xE0)
        {
            chr &= 0xF;
            chr <<= 12;
            chr += u32(string[++i] & 0x3F) << 6;
            chr += u32(string[++i] & 0x3F);
        }
        else if ((chr & 0xF8) == 0xF0)
        {
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

i32 CharLen(const char chr) {
    if (!(chr & 0x80))
    {
        return 1;
    }
    else if ((chr & 0xE0) == 0xC0)
    {
        return 2;
    }
    else if ((chr & 0xF0) == 0xE0)
    {
        return 3;
    }
    else if ((chr & 0xF8) == 0xF0)
    {
        return 4;
    }
    return 1;
}

} // namespace AzCore
