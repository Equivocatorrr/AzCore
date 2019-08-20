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
STRING_TERMINATOR(char32, 0u);

String operator+(const char* cString, const String& string) {
    String value(string);
    return cString + std::move(value);
}

String operator+(const char* cString, String&& string) {
    String result;
    result.Reserve(StringLength(cString)+string.size);
    result.Append(cString);
    result.Append(std::move(string));
    return result;
}

WString operator+(const char32* cString, const WString& string) {
    WString value(string);
    return cString + std::move(value);
}

WString operator+(const char32* cString, WString&& string) {
    WString result;
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
    out.Reserve(i32(log((f32)value) / log((f32)base))+1);
    u32 remaining = value;
    while (remaining != 0) {
        u32 quot = remaining/base;
        u32 rem  = remaining%base;
        if (base > 10) {
            out.Append(rem > 9 ? char(rem+'a'-10) : char(rem+'0'));
        } else {
            out.Append(char(rem+'0'));
        }
        remaining = quot;
    }
    return out.Reverse();
}

String ToString(const u64& value, i32 base) {
    if (value == 0) {
        return "0";
    }
    String out;
    out.Reserve(i32((f32)log((f64)value) / log((f32)base))+1);
    u64 remaining = value;
    while (remaining != 0) {
        u64 quot = remaining/base;
        u64 rem  = remaining%base;
        if (base > 10) {
            out.Append(rem > 9 ? char(rem+'a'-10) : char(rem+'0'));
        } else {
            out.Append(char(rem+'0'));
        }
        remaining = quot;
    }
    return out.Reverse();
}

String ToString(const u128& value, i32 base) {
    if (value == 0) {
        return "0";
    }
    String out;
    out.Reserve(i32((f32)log((f64)value) / log((f32)base))+1);
    u128 remaining = value;
    while (remaining != 0) {
        u128 quot = remaining/base;
        u128 rem  = remaining%base;
        if (base > 10) {
            out.Append(rem > 9 ? char(rem+'a'-10) : char(rem+'0'));
        } else {
            out.Append(char(rem+'0'));
        }
        remaining = quot;
    }
    return out.Reverse();
}

String ToString(const i32& value, i32 base) {
    if (value == 0) {
        return "0";
    }
    String out;
    out.Reserve(i32(log((f32)value) / log((f32)base))+1);
    bool negative = value < 0;
    i32 remaining = abs(value);
    while (remaining != 0) {
        i32 quot = remaining/base;
        i32 rem  = remaining%base;
        if (base > 10) {
            out.Append(rem > 9 ? char(rem+'a'-10) : char(rem+'0'));
        } else {
            out.Append(char(rem+'0'));
        }
        remaining = quot;
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
    out.Reserve(i32((f32)log((f64)value) / log((f32)base))+1);
    bool negative = value < 0;
    i64 remaining = abs(value);
    while (remaining != 0) {
        i64 quot = remaining/base;
        i64 rem  = remaining%base;
        if (base > 10) {
            out.Append(rem > 9 ? char(rem+'a'-10) : char(rem+'0'));
        } else {
            out.Append(char(rem+'0'));
        }
        remaining = quot;
    }
    if (negative) {
        out += '-';
    }
    return out.Reverse();
}

String ToString(const i128& value, i32 base) {
    if (value == 0) {
        return "0";
    }
    String out;
    out.Reserve(i32((f32)log((f64)value) / log((f32)base))+1);
    bool negative = value < 0;
    i128 remaining = abs(value);
    while (remaining != 0) {
        i128 quot = remaining/base;
        i128 rem  = remaining%base;
        if (base > 10) {
            out.Append(rem > 9 ? char(rem+'a'-10) : char(rem+'0'));
        } else {
            out.Append(char(rem+'0'));
        }
        remaining = quot;
    }
    if (negative) {
        out += '-';
    }
    return out.Reverse();
}

String ToString(const f32& value, i32 base) {
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
    String out;
    out.Reserve(12);
    f32 basis = 1.0;
    f32 remaining = value;
    if (remaining < 0.0) {
        remaining = -remaining;
        out += '-';
    }
    i32 newExponent = 0;
    bool point = false;
    if (remaining >= 1.0) {
        while(true) {
            f32 newBasis = basis * base;
            if (newBasis > remaining) {
                break;
            } else {
                newExponent++;
                basis = newBasis;
            }
        }
    } else {
        while(true) {
            basis /= base;
            newExponent--;
            if (basis <= remaining) {
                break;
            }
        }
    }
    f32 crossover;
    if (newExponent > 7 || newExponent < -1) {
        crossover = basis/base;
    } else {
        if (remaining < 1.0) {
            out += "0.";
            point = true;
            for (i32 i = 2; i < newExponent; i++) {
                out += '0';
            }
        }
        crossover = 1.0/base;
    }
    for (i32 count = 8; count > 0; count--) {
        i32 digit = remaining / basis;
        out += digit >= 10 ? (digit+'A'-10) : (digit+'0');
        remaining -= basis * (f32)digit;
        basis /= base;
        if (!point && basis <= crossover) {
            out += '.';
            point = true;
        }
    }
    i32 i = out.size-1;
    for (; out[i] == '0'; i--) {}
    if (out[i] == '.') {
        i++; // Leave 1 trailing zero
    }
    out.Resize(i+1);
    if (newExponent > 7) {
        out += "e+" + ToString(newExponent);
    } else if (newExponent < -1) {
        out += "e-" + ToString(-newExponent);
    }

    return out;
}

String ToString(const f64& value, i32 base) {
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
    String out;
    out.Reserve(24);
    f64 basis = 1.0;
    f64 remaining = value;
    if (remaining < 0.0) {
        remaining = -remaining;
        out += '-';
    }
    i32 newExponent = 0;
    bool point = false;
    if (remaining >= 1.0) {
        while(true) {
            f64 newBasis = basis * base;
            if (newBasis > remaining) {
                break;
            } else {
                newExponent++;
                basis = newBasis;
            }
        }
    } else {
        while(true) {
            basis /= base;
            newExponent--;
            if (basis <= remaining) {
                break;
            }
        }
    }
    f64 crossover;
    if (newExponent > 15 || newExponent < -1) {
        crossover = basis/base;
    } else {
        if (remaining < 1.0) {
            out += "0.";
            point = true;
            for (i32 i = 2; i < newExponent; i++) {
                out += '0';
            }
        }
        crossover = 1.0/base;
    }
    for (i32 count = 16; count > 0; count--) {
        i32 digit = remaining / basis;
        out += digit >= 10 ? (digit+'A'-10) : (digit+'0');
        remaining -= basis * (f64)digit;
        basis /= base;
        if (!point && basis <= crossover) {
            out += '.';
            point = true;
        }
    }
    i32 i = out.size-1;
    for (; out[i] == '0'; i--) {}
    if (out[i] == '.') {
        i++; // Leave 1 trailing zero
    }
    out.Resize(i+1);
    if (newExponent > 15) {
        out += "e+" + ToString(newExponent);
    } else if (newExponent < -1) {
        out += "e-" + ToString(-newExponent);
    }

    return out;
}

String ToString(const f128& value, i32 base) {
    u128 byteCode;
    memcpy((void*)&byteCode, (void*)&value, sizeof(byteCode));
    const bool negative = (byteCode >> 127) != 0;
    i16 exponent = (byteCode >> 112) & 0x7fff;
    u128 significand = (byteCode << 16) >> 16 | ((u128)1 << 112); // Get our implicit bit in there.
    if (exponent == 0x0) {
        if (significand == (u128)1 << 112) {
            return negative ? "-0.0" : "0.0";
        } else {
            significand = (byteCode << 16) >> 16; // Get that implicit bit out of here!
        }
    }
    if (exponent == 0x7fff) {
        if (significand == (u128)1 << 112) {
            return negative ? "-Infinity" : "Infinity";
        } else {
            return negative ? "-NaN" : "NaN";
        }
    }
    exponent -= 16383;
    if (exponent == 112) {
        return ToString((negative ? -(i128)significand : (i128)significand) >> (112-exponent), base) + ".0";
    }
    String out;
    out.Reserve(40);

    f128 basis = 1.0;
    f128 remaining = value;
    if (remaining < 0.0) {
        remaining = -remaining;
        out += '-';
    }
    i32 newExponent = 0;
    bool point = false;
    if (remaining >= 1.0) {
        while(true) {
            f128 newBasis = basis * base;
            if (newBasis > remaining) {
                break;
            } else {
                newExponent++;
                basis = newBasis;
            }
        }
    } else {
        while(true) {
            basis /= base;
            newExponent--;
            if (basis <= remaining) {
                break;
            }
        }
    }
    f128 crossover;
    if (newExponent > 33 || newExponent < -1) {
        crossover = basis/base;
    } else {
        if (remaining < 1.0) {
            out += "0.";
            point = true;
            for (i32 i = 2; i < newExponent; i++) {
                out += '0';
            }
        }
        crossover = 1.0/base;
    }
    for (i32 count = 34; count > 0; count--) {
        i32 digit = remaining / basis;
        out += digit >= 10 ? (digit+'A'-10) : (digit+'0');
        remaining -= basis * (f128)digit;
        basis /= base;
        if (!point && basis <= crossover) {
            out += '.';
            point = true;
        }
    }
    i32 i = out.size-1;
    for (; out[i] == '0'; i--) {}
    if (out[i] == '.') {
        i++; // Leave 1 trailing zero
    }
    out.Resize(i+1);
    if (newExponent > 33) {
        out += "e+" + ToString(newExponent);
    } else if (newExponent < -1) {
        out += "e-" + ToString(-newExponent);
    }

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
        char32 chr = string[i];
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
        out += chr;
    }
    return out;
}

String FormatTime(Nanoseconds time) {
    String out;
    u64 count = time.count();
    const u64 unitTimes[] = {UINT64_MAX, 60000000000, 1000000000, 1000000, 1000, 1};
    const char *unitStrings[] = {"m", "s", "ms", "μs", "ns"};
    bool addSpace = false;
    for (u32 i = 0; i < 5; i++) {
        if (count > unitTimes[i+1]) {
            if (addSpace) {
                out += ' ';
            }
            out += ToString((count%unitTimes[i])/unitTimes[i+1]) + unitStrings[i];
            addSpace = true;
        }
    }
    return out;
}
