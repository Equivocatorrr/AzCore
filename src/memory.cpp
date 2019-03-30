/*
    File: memory.cpp
    Author: Philip Haynes
*/

#include "memory.hpp"
#include "bigint.hpp"

STRING_TERMINATOR(char, 0);
STRING_TERMINATOR(wchar_t, 0);

String operator+(const char* cString, const String& string) {
    String value(string);
    return cString + std::move(value);
}

String operator+(const char* cString, String&& string) {
    String result(false); // We don't initialize the tail
    result.Reserve(StringLength(cString)+string.size);
    result += cString;
    result += string;
    return result;
}

WString operator+(const char* cString, const WString& string) {
    WString value(string);
    return cString + std::move(value);
}

WString operator+(const wchar_t* cString, WString&& string) {
    WString result(false); // We don't initialize the tail
    result.Reserve(StringLength(cString)+string.size);
    result += cString;
    result += string;
    return result;
}

String ToString(const u32& value, i32 base) {
    if (value == 0) {
        return "0";
    }
    String out;
    u32 remaining = value;
    while (remaining != 0) {
        div_t val = div(remaining, base);
        if (base >= 10) {
            out += val.rem > 9 ? char(val.rem-10+'a') : char(val.rem+'0');
        } else {
            out += char(val.rem+'0');
        }
        remaining = val.quot;
    }
    return out.Reverse();
}

String ToString(const u64& value, i32 base) {
    if (value == 0) {
        return "0";
    }
    String out;
    u64 remaining = value;
    while (remaining != 0) {
        lldiv_t val = lldiv(remaining, base);
        if (base >= 10) {
            out += val.rem > 9 ? char(val.rem-10+'a') : char(val.rem+'0');
        } else {
            out += char(val.rem+'0');
        }
        remaining = val.quot;
    }
    return out.Reverse();
}

String ToString(const i32& value, i32 base) {
    if (value == 0) {
        return "0";
    }
    String out;
    bool negative = value < 0;
    i32 remaining = value;
    while (remaining != 0) {
        div_t val = div(remaining, base);
        if (base > 10) {
            out += val.rem > 9 ? char(val.rem-10+'a') : char(val.rem+'0');
        } else {
            out += char(val.rem+'0');
        }
        remaining = val.quot;
    }
    if (negative) {
        out += '-';
    }
    return out.Reverse();
}

String ToString(const i64& value, i32 base) {
    if (value == 0) {
        return "0";
    }
    String out;
    bool negative = value < 0;
    i64 remaining = value;
    while (remaining != 0) {
        lldiv_t val = lldiv(remaining, base);
        if (base >= 10) {
            out += val.rem > 9 ? char(val.rem-10+'a') : char(val.rem+'0');
        } else {
            out += char(val.rem+'0');
        }
        remaining = val.quot;
    }
    if (negative) {
        out += '-';
    }
    return out.Reverse();
}

#include <cstdio> // For sprintf, as long as my own float to string implementation doesn't quite work.

String ToString(const f32& value, i32 base) {
    // TODO: Finish this implementation since sprintf can't do bases other than 10 and 16
    u32 byteCode;
    memcpy((void*)&byteCode, (void*)&value, sizeof(byteCode));
    const bool negative = (byteCode & 0x80000000) != 0;
    u32 exponent = (byteCode >> 23) & 0xff;
    u32 significand = (byteCode & 0x007fffff) | (0x00800000); // Get our implicit bit in there.
    if (exponent == 0x00) {
        if (significand == 0x00800000) {
            return negative ? "-0.0" : "0.0";
        } else {
            significand &= 0x007fffff; // Get that implicit bit out of here!
        }
    }
    if (exponent == 0xff) {
        if (significand == 0x00800000) {
            return negative ? "-Infinity" : "Infinity";
        } else {
            return negative ? "-NaN" : "NaN";
        }
    }
    if (exponent == 150) {
        return ToString(negative ? -(i32)significand : (i32)significand, base) + ".0";
    }
    /*
    const f32 log2Base = log2((f32)base);
    String out;
    BigInt iPart(significand);
    iPart <<= (i32)exponent-150 + 150.0*log2Base;
    if (iPart == 0) {
        out = "0";
    }
    i32 fractionalExponent = u32(((f32)exponent - 150.0 + (150.0*log2Base)) / log2Base);
    i32 decimalExponent = u32(127.0 / log2Base);
    while (iPart != 0 || fractionalExponent > 0) {
        if (fractionalExponent-- == decimalExponent) {
            out += '.';
        }
        u32 remainder;
        BigInt::QuotientAndRemainder(iPart, base, &iPart, &remainder);
        if (base >= 10) {
            out += remainder > 9 ? char(remainder-10+'a') : char(remainder+'0');
        } else {
            out += char(remainder+'0');
        }
    }
    if (negative) {
        out += '-';
    }
    return out.Reverse();
    */
    String out(false);
    char buffer[64]; // Way more space than a single-precision float should ever need.
    sprintf(buffer, "%f", (f64)value);
    out += buffer;
    i32 i = out.size-1;
    for (; out[i] == '0'; i--) {}
    if (out[i] == '.') {
        i++; // Leave 1 trailing zero
    }
    out.Resize(i+1);
    return out;
}

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
        u32 chr = string[i];
        if (!(chr & 0x80)) {
            chr &= 0x7F;
        } else if ((chr & 0xE0) == 0xC0) {
            chr &= 0x1F;
            chr <<= 6;
            chr += u32(string[++i]&0x3F);
        } else if ((chr & 0xF0) == 0xE0) {
            chr &= 0xF;
            chr <<= 12;
            chr += u32(string[++i]&0x3F) << 6;
            chr += u32(string[++i]&0x3F);
        } else if ((chr & 0xF8) == 0xF0) {
            chr &= 0x7;
            chr <<= 18;
            chr += u32(string[++i]&0x3F) << 12;
            chr += u32(string[++i]&0x3F) << 6;
            chr += u32(string[++i]&0x3F);
        }
        out += (wchar_t)chr;
    }
    return out;
}
