/*
    File: bigint.hpp
    Author: Philip Haynes
    An arbitrary-precision integer class that uses a string of
    32-bit integers to represent an integer with any number of digits.
*/
#ifndef BIGINT_HPP
#define BIGINT_HPP

#define BIGINT_BUCKET_SIZE 30

#include "common.hpp"

class BigInt {
public:
    bool negative;
    BucketArray<u32, BIGINT_BUCKET_SIZE> words;
    inline BigInt() : negative(false), words(1, 0) {}
    inline BigInt(const BigInt& a) : negative(a.negative), words(a.words) {}
    explicit inline BigInt(const u64& a, bool neg=false) { *this = a; negative = neg; }
    explicit inline BigInt(const u32& a, bool neg=false) { *this = a; negative = neg; }
    explicit inline BigInt(const i64& a) { *this = a; }
    explicit inline BigInt(const i32& a) { *this = a; }
    explicit inline BigInt(const BucketArray<u32, BIGINT_BUCKET_SIZE>& init, bool neg=false) : negative(neg), words(init) {}
    explicit BigInt(const String& string, bool neg=false, const u32 base=10);
    inline BigInt& operator=(const BigInt& a) {
        words = a.words;
        negative = a.negative;
        return *this;
    }
    inline BigInt& operator=(const u64& a) {
        u32 a2 = a >> 32;
        if (a2 != 0) {
            words = {(u32)a, a2};
        } else {
            words = {(u32)a};
        }
        return *this;
    }
    inline BigInt& operator=(const u32& a) {
        words = {a};
        return *this;
    }
    inline BigInt& operator=(const i64& a) {
        u64 aa = (*((u64*)(&a))) & 0x7fffffffffffffff;
        u32 a2 = aa >> 32;
        if (a2 != 0) {
            words = {(u32)a, a2};
        } else {
            words = {(u32)a};
        }
        negative = a < 0;
        return *this;
    }
    inline BigInt& operator=(const i32& a) {
        u32 aa = (*((u32*)(&a))) & 0x7fffffff; // reinterpret_cast doesn't let me do this >:(
        words = {aa};
        negative = a < 0;
        return *this;
    }

    bool operator>(const BigInt& a) const;
    bool operator>=(const BigInt& a) const;
    bool operator<(const BigInt& a) const;
    bool operator<=(const BigInt& a) const;
    inline bool operator==(const BigInt& a) const {
        return negative == a.negative && words == a.words;
    }

    bool operator>(const u32& a) const;
    bool operator>=(const u32& a) const;
    bool operator<(const u32& a) const;
    bool operator<=(const u32& a) const;
    inline bool operator==(const u32& a) const {
        return words.size == 1 && negative == false && words[0] == a;
    }

    inline bool operator!=(const BigInt& a) const {
        return !(*this == a);
    }

    inline bool operator!=(const u32& a) const {
        return !(*this == a);
    }

    BigInt operator-() const;

    BigInt& operator+=(const BigInt& a);
    BigInt& operator-=(const BigInt& a);
    BigInt& operator*=(const BigInt& a);
    BigInt& operator/=(const BigInt& a);
    BigInt& operator%=(const BigInt& a);

    BigInt& operator+=(const u32& a);
    BigInt& operator-=(const u32& a);
    BigInt& operator*=(const u32& a);
    BigInt& operator/=(const u32& a);
    BigInt& operator%=(const u32& a);

    inline BigInt operator+(const BigInt& a) const {
        BigInt temp(*this);
        temp += a;
        return temp;
    }
    inline BigInt operator-(const BigInt& a) const {
        BigInt temp(*this);
        temp -= a;
        return temp;
    }
    inline BigInt operator*(const BigInt& a) const {
        BigInt temp(*this);
        temp *= a;
        return temp;
    }
    inline BigInt operator/(const BigInt& a) const {
        BigInt temp(*this);
        temp /= a;
        return temp;
    }
    inline BigInt operator%(const BigInt& a) const {
        BigInt temp(*this);
        temp %= a;
        return temp;
    }

    inline BigInt operator+(const u32& a) const {
        BigInt temp(*this);
        temp += a;
        return temp;
    }
    inline BigInt operator-(const u32& a) const {
        BigInt temp(*this);
        temp -= a;
        return temp;
    }
    inline BigInt operator*(const u32& a) const {
        BigInt temp(*this);
        temp *= a;
        return temp;
    }
    inline BigInt operator/(const u32& a) const {
        BigInt temp(*this);
        temp /= a;
        return temp;
    }
    inline BigInt operator%(const u32& a) const {
        BigInt temp(*this);
        temp %= a;
        return temp;
    }

    static void QuotientAndRemainder(const BigInt& a, const BigInt& b, BigInt *dstQuotient, BigInt *dstRemainder);
    static void QuotientAndRemainder(const BigInt& a, const u32& b, BigInt *dstQuotient, u32 *dstRemainder);

    BigInt& operator<<=(i32 i);
    BigInt& operator>>=(i32 i);

    inline BigInt operator<<(const i32& i) const {
        BigInt temp(*this);
        temp <<= i;
        return temp;
    }
    inline BigInt operator>>(const i32& i) const {
        BigInt temp(*this);
        temp >>= i;
        return temp;
    }

    void Trim(); // Gets rid of trailing zeroes.
    BigInt Trimmed() const;

    String Digits(const i32 base=10) const; // Gets all the digits in no particular order
    String HexString() const;
};

static_assert(sizeof(BigInt) == 128);

String ToString(const BigInt& value, const i32 base=10);

template<typename T>
inline bool operator>(const T& a, const BigInt& b) {
    return b < a;
}
template<typename T>
inline bool operator<(const T& a, const BigInt& b) {
    return b > a;
}
template<typename T>
inline bool operator>=(const T& a, const BigInt& b) {
    return b <= a;
}
template<typename T>
inline bool operator<=(const T& a, const BigInt& b) {
    return b >= a;
}
template<typename T>
inline bool operator==(const T& a, const BigInt& b) {
    return b == a;
}

template<typename T>
inline BigInt operator+(const T& a, const BigInt& b) {
    return b + a;
}
template<typename T>
inline BigInt operator-(const T& a, const BigInt& b) {
    return BigInt(a) - b;
}
template<typename T>
inline BigInt operator*(const T& a, const BigInt& b) {
    return b * a;
}
template<typename T>
inline BigInt operator/(const T& a, const BigInt& b) {
    return BigInt(a) / b;
}

inline BigInt abs(const BigInt& a) {
    BigInt newValue(a.words);
    return newValue;
}

#endif
