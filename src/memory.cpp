/*
    File: memory.cpp
    Author: Philip Haynes
*/

#include "memory.hpp"
#include "bigint.hpp"

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

String ToString(f32 value, i32 base) {
    const u32 byteCode = *((u32*)((void*)&value));
    const bool negative = (byteCode & 0x80000000) != 0;
    const u8 exponent = byteCode >> 23;
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
    if (exponent == 127) {
        return ToString(negative ? -(i32)significand : (i32)significand, base) + ".0";
    }
    String iString, fString;
    if (exponent > 103) {
        BigInt iPart(significand);
        iPart <<= (i32)exponent-127;
        while (iPart != 0) {
            u32 remainder;
            BigInt::QuotientAndRemainder(iPart, base, &iPart, &remainder);
            if (base >= 10) {
                iString += remainder > 9 ? char(remainder-10+'a') : char(remainder+'0');
            } else {
                iString += char(remainder+'0');
            }
        }
    } else {
        // Integer part is 0
        iString = "0";
    }
    if (exponent < 127) {
        BigInt fPart(significand);
        fPart <<= exponent;
        fPart = fPart - ((fPart >> 127) << 127);
        if (fPart == 0) {
            fString = "0";
        }
        while (fPart != 0) {
            u32 remainder;
            BigInt::QuotientAndRemainder(fPart, base, &fPart, &remainder);
            if (base >= 10) {
                fString += remainder > 9 ? char(remainder-10+'a') : char(remainder+'0');
            } else {
                fString += char(remainder+'0');
            }
        }
    } else {
        // Fractional part is 0
        fString = "0";
    }
    if (negative) {
        return "-" + iString + "." + fString;
    } else {
        return iString + "." + fString;
    }
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
