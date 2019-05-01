/*
    File: memory.cpp
    Author: Philip Haynes
*/

#include "memory.hpp"
#include "bigint.hpp"

SystemEndianness_t SysEndian{1};

u16 bytesToU16(char bytes[2], bool swapEndian) {
    union {
        char bytes[2];
        u16 val;
    } out;
    if (swapEndian) {
        for (u32 i = 0; i < 2; i++) {
            out.bytes[i] = bytes[1-i];
        }
    } else {
        out.val = *(u16*)bytes;
    }
    return out.val;
}
u32 bytesToU32(char bytes[4], bool swapEndian) {
    union {
        char bytes[4];
        u32 val;
    } out;
    if (swapEndian) {
        for (u32 i = 0; i < 4; i++) {
            out.bytes[i] = bytes[3-i];
        }
    } else {
        out.val = *(u32*)bytes;
    }
    return out.val;
}
u64 bytesToU64(char bytes[8], bool swapEndian) {
    union {
        char bytes[8];
        u64 val;
    } out;
    if (swapEndian) {
        for (u32 i = 0; i < 8; i++) {
            out.bytes[i] = bytes[7-i];
        }
    } else {
        out.val = *(u64*)bytes;
    }
    return out.val;
}

i16 bytesToI16(char bytes[2], bool swapEndian) {
    union {
        char bytes[2];
        i16 val;
    } out;
    if (swapEndian) {
        for (u32 i = 0; i < 2; i++) {
            out.bytes[i] = bytes[1-i];
        }
    } else {
        out.val = *(i16*)bytes;
    }
    return out.val;
}
i32 bytesToI32(char bytes[4], bool swapEndian) {
    union {
        char bytes[4];
        i32 val;
    } out;
    if (swapEndian) {
        for (u32 i = 0; i < 4; i++) {
            out.bytes[i] = bytes[3-i];
        }
    } else {
        out.val = *(i32*)bytes;
    }
    return out.val;
}
i64 bytesToI64(char bytes[8], bool swapEndian) {
    union {
        char bytes[8];
        i64 val;
    } out;
    if (swapEndian) {
        for (u32 i = 0; i < 8; i++) {
            out.bytes[i] = bytes[7-i];
        }
    } else {
        out.val = *(i64*)bytes;
    }
    return out.val;
}

f32 bytesToF32(char bytes[4], bool swapEndian) {
    union {
        char bytes[4];
        f32 val;
    } out;
    if (swapEndian) {
        for (u32 i = 0; i < 4; i++) {
            out.bytes[i] = bytes[3-i];
        }
    } else {
        out.val = *(f32*)bytes;
    }
    return out.val;
}
f64 bytesToF64(char bytes[8], bool swapEndian) {
    union {
        char bytes[8];
        f64 val;
    } out;
    if (swapEndian) {
        for (u32 i = 0; i < 8; i++) {
            out.bytes[i] = bytes[7-i];
        }
    } else {
        out.val = *(f64*)bytes;
    }
    return out.val;
}

u16 endianSwap(u16 in, bool swapEndian) {
    if (!swapEndian) {
        return in;
    }
    union {
        char bytes[2];
        u16 val;
    } b_in, b_out;
    b_in.val = in;
    for (u32 i = 0; i < 2; i++) {
        b_out.bytes[i] = b_in.bytes[1-i];
    }
    return b_out.val;
}

u32 endianSwap(u32 in, bool swapEndian) {
    if (!swapEndian) {
        return in;
    }
    union {
        char bytes[4];
        u32 val;
    } b_in, b_out;
    b_in.val = in;
    for (u32 i = 0; i < 4; i++) {
        b_out.bytes[i] = b_in.bytes[3-i];
    }
    return b_out.val;
}

u64 endianSwap(u64 in, bool swapEndian) {
    if (!swapEndian) {
        return in;
    }
    union {
        char bytes[8];
        u64 val;
    } b_in, b_out;
    b_in.val = in;
    for (u32 i = 0; i < 8; i++) {
        b_out.bytes[i] = b_in.bytes[7-i];
    }
    return b_out.val;
}

size_t align(const size_t& size, const size_t& alignment) {
    // if (size % alignment == 0) {
    //     return size;
    // } else {
    //     return (size/alignment+1)*alignment;
    // }
    return (size + alignment-1) & ~(alignment-1);
}

STRING_TERMINATOR(char, '\0');
STRING_TERMINATOR(wchar_t, L'\0');

String operator+(const char* cString, const String& string) {
    String value(string);
    return cString + std::move(value);
}

String operator+(const char* cString, String&& string) {
    String result(false); // We don't initialize the tail
    result.Reserve(StringLength(cString)+string.size);
    result.Append(cString);
    result.Append(std::move(string));
    return result;
}

WString operator+(const char* cString, const WString& string) {
    WString value(string);
    return cString + std::move(value);
}

WString operator+(const wchar_t* cString, WString&& string) {
    WString result(false); // We don't initialize the tail
    result.Reserve(StringLength(cString)+string.size);
    result.Append(cString);
    result.Append(std::move(string));
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
    // String iString(false), fString(false);
    // if (exponent >= 127) {
    //     BigInt iPart(significand);
    //     iPart <<= (i32)exponent-150;
    //     iString = ToString(iPart, base);
    // } else {
    //     iString = "0";
    // }
    // if (exponent < 150) {
    //     u32 significandReversed = 0;
    //     for (u32 i = 0; i < 24; i++) {
    //         if (significand & (1 << i)) {
    //             significandReversed |= 1 << (23-i);
    //         }
    //     }
    //     // significandReversed = (1 << 24) - significandReversed;
    //     BigInt fPart(significandReversed);
    //     fPart >>= (i32)exponent - 126;
    //     if (fPart == 0) {
    //         fString = "0";
    //     } else {
    //         fString = ToString(fPart, base).Reverse() + " " + ToString(significandReversed, 16);
    //     }
    // } else {
    //     fString = "0";
    // }
    // if (negative) {
    //     return "-" + std::move(iString) + "." + std::move(fString) + " " + ToString(significand, 16);
    // } else {
    //     return std::move(iString) + "." + std::move(fString) + " " + ToString(significand, 16);
    // }
    String out(false);
    char buffer[64];
    i32 i = sprintf(buffer, "%.8f", value) - 1;
    for (; buffer[i] == '0'; i--) {}
    if (buffer[i] == '.') {
        i++; // Leave 1 trailing zero
    }
    buffer[i+1] = 0;
    out += buffer;
    return out;
}

String ToString(const f64& value, i32 base) {
    // TODO: Finish this implementation since sprintf can't do bases other than 10 and 16
    u64 byteCode;
    memcpy((void*)&byteCode, (void*)&value, sizeof(byteCode));
    const bool negative = (byteCode & 0x8000000000000000) != 0;
    u32 exponent = (byteCode >> 52) & 0x7ff;
    u64 significand = (byteCode & 0x000fffffffffffff) | (0x0010000000000000); // Get our implicit bit in there.
    if (exponent == 0x0) {
        if (significand == 0x0010000000000000) {
            return negative ? "-0.0" : "0.0";
        } else {
            significand &= 0x000fffffffffffff; // Get that implicit bit out of here!
        }
    }
    if (exponent == 0x7ff) {
        if (significand == 0x0010000000000000) {
            return negative ? "-Infinity" : "Infinity";
        } else {
            return negative ? "-NaN" : "NaN";
        }
    }
    if (exponent == 1075) {
        return ToString(negative ? -(i64)significand : (i64)significand, base) + ".0";
    }
    String out(false);
    char buffer[128];
    i32 i = sprintf(buffer, "%.16f", value) - 1;
    for (; buffer[i] == '0'; i--) {}
    if (buffer[i] == '.') {
        i++; // Leave 1 trailing zero
    }
    buffer[i+1] = 0;
    out += buffer;
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
