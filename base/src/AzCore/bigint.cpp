#include "bigint.hpp"

BigInt::BigInt(const String& string, bool neg, const u32 base) : words(), negative(false) {
    BigInt mul(1u);
    u32 mulCache = 1;
    u32 cache = 0;
    for (i32 i = string.size-1; i >= 0; i--) {
        const char& c = string[i];
        if (base <= 10) { // This should get optimized out
            cache += u32(c-'0') * mulCache;
        } else {
            cache += u32(c < 'a' ? (c-'0') : (c-'a'+10)) * mulCache;
        }
        mulCache *= base;
        if (mulCache > UINT32_MAX/base) {
            *this += mul * cache;
            mul *= mulCache;
            mulCache = 1;
            cache = 0;
        }
    }
    if (cache != 0) {
        *this += mul * cache;
    }
    negative = neg;
}

bool BigInt::operator>(const BigInt& a) const {
    if (*this == a) {
        return false;
    }
    if (negative != a.negative) {
        return a.negative;
    } else {
        if (words.size > a.words.size) {
            return !negative;
        } else if (words.size < a.words.size) {
            return negative;
        } else {
            for (i32 i = words.size-1; i >= 0; i--) {
                if (words[i] > a.words[i]) {
                    return !negative;
                } else if (words[i] < a.words[i]) {
                    return negative;
                }
            }
        }
    }
    return false;
}

bool BigInt::operator>=(const BigInt& a) const {
    if (negative != a.negative) {
        return a.negative;
    } else {
        if (words.size > a.words.size) {
            return !negative;
        } else if (words.size < a.words.size) {
            return negative;
        } else {
            for (i32 i = words.size-1; i >= 0; i--) {
                if (words[i] > a.words[i]) {
                    return !negative;
                } else if (words[i] < a.words[i]) {
                    return negative;
                }
            }
        }
    }
    return true;
}

bool BigInt::operator<(const BigInt& a) const {
    if (*this == a) {
        return false;
    }
    if (negative != a.negative) {
        return negative;
    } else {
        if (words.size < a.words.size) {
            return !negative;
        } else if (words.size > a.words.size) {
            return negative;
        } else {
            for (i32 i = words.size-1; i >= 0; i--) {
                if (words[i] < a.words[i]) {
                    return !negative;
                } else if (words[i] > a.words[i]) {
                    return negative;
                }
            }
        }
    }
    return false;
}

bool BigInt::operator<=(const BigInt& a) const {
    if (negative != a.negative) {
        return negative;
    } else {
        if (words.size < a.words.size) {
            return !negative;
        } else if (words.size > a.words.size) {
            return negative;
        } else {
            for (i32 i = words.size-1; i >= 0; i--) {
                if (words[i] < a.words[i]) {
                    return !negative;
                } else if (words[i] > a.words[i]) {
                    return negative;
                }
            }
        }
    }
    return true;
}

bool BigInt::operator>(const u64& a) const {
    if (*this == a) {
        return false;
    }
    if (negative) {
        return false;
    } else {
        if (words.size == 0) {
            return false;
        } else if (words.size > 1) {
            return !negative;
        } else {
            if (words[0] > a) {
                return !negative;
            } else if (words[0] < a) {
                return negative;
            }
        }
    }
    return false;
}

bool BigInt::operator>=(const u64& a) const {
    if (negative) {
        return false;
    } else {
        if (words.size == 0) {
            return false;
        } else if (words.size > 1) {
            return !negative;
        } else {
            if (words[0] > a) {
                return !negative;
            } else if (words[0] < a) {
                return negative;
            }
        }
    }
    return true;
}

bool BigInt::operator<(const u64& a) const {
    if (*this == a) {
        return false;
    }
    if (negative) {
        return true;
    } else {
        if (words.size == 0) {
            return true;
        } else if (words.size > 1) {
            return negative;
        } else {
            if (words[0] < a) {
                return !negative;
            } else if (words[0] > a) {
                return negative;
            }
        }
    }
    return false;
}

bool BigInt::operator<=(const u64& a) const {
    if (negative) {
        return true;
    } else {
        if (words.size == 0) {
            return true;
        } else if (words.size > 1) {
            return negative;
        } else {
            if (words[0] < a) {
                return !negative;
            } else if (words[0] > a) {
                return negative;
            }
        }
    }
    return true;
}

BigInt BigInt::operator-() const {
    BigInt temp(*this);
    temp.negative = !negative;
    return temp;
}

BigInt& BigInt::operator+=(const BigInt& a) {
    if (a.negative != negative) {
        if (negative) {
            return *this = a - -*this;
        } else {
            return *this -= -a;
        }
    }
    if (a.words.size > words.size) {
        words.Resize(a.words.size, 0);
    }
    u128 dword = 0; // Used to carry between words.
    i32 i;
    for (i = 0; i < words.size; i++) {
        if (i < a.words.size) {
            dword += a.words[i];
        }
        dword += words[i];
        words[i] = (u64)dword;
        dword >>= 64;
    }
    if (dword != 0) {
        words.Append((u64)dword);
    }
    return *this;
}

BigInt& BigInt::operator-=(const BigInt& a) {
    if (negative != a.negative) {
        return *this += -a;
    }
    if (a == *this) {
        words = {0};
        negative = false;
        return *this; // They cancel each other out
    } else if (abs(*this) > abs(a)) {
        // From this point we're guaranteed that we have at least as many words as "a" does
        bool carry = false; // We only need to know about one bit between words.
        u64 word = 0;
        for (i32 i = 0; i < words.size; i++) {
            word = words[i];
            if (carry) {
                if (word != 0) {
                    carry = false;
                }
                word--;
            }
            if (i < a.words.size) {
                if (a.words[i] > word) {
                    carry = true;
                }
                word -= a.words[i];
            }
            words[i] = word;
        }
    } else {
        negative = !negative;
        // From this point we're guaranteed that "a" has at least as many words as we do
        bool carry = false; // We only need to know about one bit between words.
        u64 word = 0;
        for (i32 i = 0; i < a.words.size; i++) {
            word = a.words[i];
            if (carry) {
                if (word != 0) {
                    carry = false;
                }
                word--;
            }
            if (i < words.size) {
                if (words[i] > word) {
                    carry = true;
                }
                words[i] = word - words[i];
            } else {
                words.Append(word);
            }
        }
    }
    return *this;
}

BigInt& BigInt::operator*=(const BigInt& a) {
    if (a == 1) {
        return *this;
    } else if (a == 0) {
        words = {0};
        negative = false;
        return *this;
    }
    bool neg = negative != a.negative;
    negative = false;
    BucketArray<u64, BIGINT_BUCKET_SIZE> wordsTemp(words);
    words = {0};
    for (i32 i = 0; i < wordsTemp.size; i++) {
        for (i32 j = 0; j < a.words.size; j++) {
            const u128 mul = (u128)a.words[j] * (u128)wordsTemp[i];
            *this += BigInt(mul) << (64*(i+j));
        }
    }
    negative = neg;
    return *this;
}

BigInt& BigInt::operator/=(const BigInt& a) {
    if (a == 0) {
        throw std::invalid_argument("Divide by zero error");
    }
    bool neg = negative != a.negative;
    negative = false;
    BigInt dividend(words);
    BigInt divisor(a.words);
    if (divisor > *this) {
        return *this = 0u;
    }
    if (divisor == *this) {
        return *this = 1u;
    }
    BigInt taken(0);
    i32 startingI = words.size*64-1;
    for (; (words[startingI/64] & (1ull << startingI%64)) == 0;) { startingI--; }
    for (i32 i = 0; i < words.size; i++) {
        words[i] = 0;
    }
    BigInt shifted = divisor << startingI;
    for (i32 i = startingI; i >= 0; i--) {
        BigInt added = taken + shifted;
        shifted >>= 1;
        if (added <= dividend) {
            taken = added;
            words[i/64] |= 1ull << (i%64);
        }
    }
    negative = neg;
    Trim();
    return *this;
}

BigInt& BigInt::operator%=(const BigInt& a) {
    if (a == 0) {
        throw std::invalid_argument("Divide by zero error");
    }
    negative = false;
    BigInt divisor(a.words);
    if (divisor > *this) {
        return *this;
    }
    if (divisor == *this) {
        return *this = 0u;
    }
    BigInt taken(0u);
    i32 startingI = words.size*64-1;
    for (; (words[startingI/64] & (1ull << startingI%64)) == 0;) { startingI--; }
    BigInt shifted = divisor << startingI;
    for (i32 i = startingI; i >= 0; i--) {
        BigInt added = taken + shifted;
        shifted >>= 1;
        if (added <= *this) {
            taken = added;
        }
    }
    *this -= taken;
    Trim();
    return *this;
}

void BigInt::QuotientAndRemainder(const BigInt &a, const BigInt &b, BigInt *dstQuotient, BigInt *dstRemainder) {
    if (b == 0) {
        throw std::invalid_argument("Divide by zero error");
    }
    BigInt dividend(a.words);
    BigInt divisor(b.words);
    if (divisor > dividend) {
        *dstQuotient = 0u;
        *dstRemainder = dividend;
        return;
    }
    if (divisor == dividend) {
        *dstQuotient = BigInt(1u, a.negative != b.negative);
        *dstRemainder = 0u;
        return;
    }
    BigInt taken(0u);
    dstQuotient->words = BucketArray<u64, BIGINT_BUCKET_SIZE>(a.words.size, 0);
    i32 startingI = a.words.size*64-1;
    for (; (a.words[startingI/64] & (1ull << startingI%64)) == 0;) { startingI--; }
    BigInt shifted = divisor << startingI;
    for (i32 i = startingI; i >= 0; i--) {
        BigInt added = taken + shifted;
        shifted >>= 1;
        if (added <= dividend) {
            taken = added;
            dstQuotient->words[i/64] |= 1ull << i%64;
        }
    }
    dstQuotient->negative = a.negative;
    dstQuotient->Trim();
    *dstRemainder = (dividend-taken).Trimmed();
}

BigInt& BigInt::operator+=(const u64& a) {
    if (negative) {
        return *this = (BigInt(a) - -*this);
    }
    i32 newSize = max((u32)words.size, a == 0u ? 0u : 1u);
    if (newSize == 0) {
        return *this;
    }
    words.Resize(newSize, 0);
    u128 dword = a; // Used to carry between words.
    i32 i;
    for (i = 0; i < words.size; i++) {
        dword += words[i];
        words[i] = dword;
        dword >>= 64;
        if (dword == 0) {
            break;
        }
    }
    if (dword != 0) {
        words.Append(dword);
    }
    return *this;
}

BigInt& BigInt::operator-=(const u64& a) {
    if (negative) {
        return *this += -BigInt(a);
    }
    if (a == *this) {
        return *this = 0u; // They cancel each other out
    } else if (abs(*this) > a) {
        bool carry = false; // We only need to know about one bit between words.
        u64 word = 0;
        for (i32 i = 0; i < words.size; i++) {
            word = words[i];
            if (carry) {
                if (word != 0) {
                    carry = false;
                }
                word--;
            }
            if (i == 0) {
                if (a > word) {
                    carry = true;
                }
                word -= a;
            }
            words[i] = word;
        }
        if (words.size == 0) {
            words.Append(a);
        }
    } else {
        negative = !negative;
        bool carry = false; // We only need to know about one bit between words.
        u64 word = a;
        for (i32 i = 0; i < words.size; i++) {
            if (carry) {
                if (word != 0) {
                    carry = false;
                }
                word--;
            }
            if (i < words.size) {
                if (words[i] > word) {
                    carry = true;
                }
                word -= words[i];
            }
            words[i] = word;
        }
        if (words.size == 0) {
            words.Append(a);
        }
    }
    return *this;
}

BigInt& BigInt::operator*=(const u64& a) {
    if (a == 1) {
        return *this;
    } else if (a == 0) {
        words = {0};
        negative = false;
        return *this;
    } else if (*this == 0) {
        return *this;
    }
    BucketArray<u64, BIGINT_BUCKET_SIZE> wordsTemp = words;
    words = {0};
    for (i32 i = 0; i < wordsTemp.size; i++) {
        const u128 mul = (u128)wordsTemp[i] * (u128)a;
        *this += BigInt(mul) << (64*i);
    }
    return *this;
}

BigInt& BigInt::operator/=(const u64& a) {
    if (a == 0) {
        throw std::invalid_argument("Divide by zero error");
    }
    bool neg = negative;
    negative = false;
    if (a > *this) {
        return *this = 0u;
    }
    if (a == *this) {
        return *this = 1u;
    }
    BigInt divisor(a);
    BigInt taken(0u);
    i32 startingI = words.size*64-1;
    for (; (words[startingI/64] & (1ull << startingI%64)) == 0;) { startingI--; }
    for (i32 i = 0; i < words.size; i++) {
        words[i] = 0;
    }
    BigInt shifted = divisor << startingI;
    for (i32 i = startingI; i >= 0; i--) {
        BigInt added = taken + shifted;
        shifted >>= 1;
        if (added <= *this) {
            taken = added;
            words[i/64] |= 1ull << i%64;
        }
    }
    negative = neg;
    Trim();
    return *this;
}

BigInt& BigInt::operator%=(const u64& a) {
    if (a == 0) {
        throw std::invalid_argument("Divide by zero error");
    }
    negative = false;
    if (a > *this) {
        return *this = 0u;
    }
    if (a == *this) {
        return *this = 1u;
    }
    BigInt divisor(a);
    BigInt taken(0u);
    i32 startingI = words.size*64-1;
    for (; (words[startingI/64] & (1ull << startingI%64)) == 0;) { startingI--; }
    BigInt shifted = divisor << startingI;
    for (i32 i = startingI; i >= 0; i--) {
        BigInt added = taken + shifted;
        shifted >>= 1;
        if (added <= *this) {
            taken = added;
        }
    }
    *this -= taken;
    Trim();
    return *this;
}

void BigInt::QuotientAndRemainder(const BigInt &a, const u64 &b, BigInt *dstQuotient, u64 *dstRemainder) {
    if (b == 0) {
        throw std::invalid_argument("Divide by zero error");
    }
    if (a == 0) {
        *dstQuotient = 0u;
        *dstRemainder = 0u;
        return;
    }
    BigInt dividend(a.words);
    if (b > dividend) {
        *dstQuotient = 0u;
        *dstRemainder = dividend.words[0];
        return;
    }
    if (b == dividend) {
        *dstQuotient = BigInt(1u, a.negative);
        *dstRemainder = 0u;
        return;
    }
    const BigInt divisor(b);
    BigInt taken(0u);
    dstQuotient->words = BucketArray<u64, BIGINT_BUCKET_SIZE>(a.words.size, 0);
    i32 startingI = dividend.words.size*64-1;
    for (; (dividend.words[startingI/64] & (1ull << startingI%64)) == 0;) { startingI--; }
    BigInt shifted = divisor << startingI;
    for (i32 i = startingI; i >= 0; i--) {
        BigInt added = taken + shifted;
        shifted >>= 1;
        if (added <= dividend) {
            taken = added;
            dstQuotient->words[i/64] |= (1ull << i%64);
        }
    }
    dstQuotient->negative = a.negative;
    dstQuotient->Trim();
    *dstRemainder = (dividend-taken).words[0];
}

BigInt& BigInt::operator<<=(i32 i) {
    if (i < 0) {
        return operator>>=(-i);
    }
    for (; i >= 64; i -= 64) {
        words.Insert(0, 0);
    }
    if (i == 0)
        return *this;
    // Now it's time for some hacky bullshittery
    u64 carry = 0;
    for (i32 x = 0; x < words.size; x++) {
        u64 buf = (words[x] << i) | carry;
        carry = words[x] >> (64-i);
        words[x] = buf;
    }
    if (carry != 0) {
        words.Append(carry);
    }
    return *this;
}

BigInt& BigInt::operator>>=(i32 i) {
    if (i < 0) {
        return operator<<=(-i);
    }
    if (i/64 >= words.size) {
        words.Resize(1);
        words[0] = 0;
        return *this;
    }
    for (; i >= 64; i -= 64) {
        words.Erase(0);
    }
    if (i == 0)
        return *this;
    u64 carry = 0;
    for (i32 x = words.size-1; x >= 0; x--) {
        u64 buf = (words[x] >> i) | carry;
        carry = words[x] << (64-i);
        words[x] = buf;
    }
    if (words.size > 1 && words[words.size-1] == 0) {
        words.Erase(words.size-1);
    }
    if (words.size == 0) {
        words = {0};
    }
    return *this;
}

void BigInt::Trim() {
    i32 newSize = words.size-1;
    for (; newSize >= 0; newSize--) {
        if (words[newSize] != 0) {
            break;
        }
    }
    words.Resize(newSize+1);
}

BigInt BigInt::Trimmed() const {
    BigInt trimmed = *this;
    trimmed.Trim();
    return trimmed;
}

String ToString(const BigInt& value, const i32 base) {
    String tmp, out;
    if (value.words.size == 0) {
        return "0";
    }
    BigInt remaining(value);
    while (remaining != 0u) {
        u64 remainder;
        BigInt::QuotientAndRemainder(remaining, base, &remaining, &remainder);
        if (base >= 10) { // This should get optimized out
            tmp += remainder > 9 ? char(remainder-10+'a') : char(remainder+'0');
        } else {
            tmp += char(remainder+'0');
        }
    }
    out.Resize(tmp.size + (value.negative ? 1 : 0));
    u32 i = 0;
    if (value.negative) {
        out[i++] = '-';
    }
    for (i32 j = tmp.size-1; j >= 0; j--, i++) {
        out[i] = tmp[j];
    }
    return out;
}

String BigInt::Digits(const i32 base) const {
    String out;
    if (words.size == 0) {
        return "0";
    }
    BigInt remaining(words);
    while (remaining != 0u) {
        u64 remainder;
        QuotientAndRemainder(remaining, base, &remaining, &remainder);
        if (base >= 10) {
            out += remainder > 9 ? char(remainder-10+'a') : char(remainder+'0');
        } else {
            out += char(remainder+'0');
        }
    }
    return out;
}

String BigInt::HexString() const {
    String string;
    if (negative) {
        string = "-0x";
    } else {
        string = " 0x";
    }
    if (words.size == 0) {
        string += "0";
    }
    for (i32 i = words.size-1; i >= 0; i--) {
        for (i32 j = 15; j >= 0; j--) {
            if ((words[i] & (0xFull << (j*4))) == 0) {
                string += "0";
            } else {
                u8 val = (words[i] >> (j*4)) & 0xF;
                string += char(val + (val > 9 ? ('A'-10) : '0'));
            }
        }
    }
    return string;
}
