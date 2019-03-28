/*
    File: bigint.hpp
    Author: Philip Haynes
    An arbitrary-precision integer class that uses a string of
    32-bit integers to represent an integer with any number of digits.
*/
#ifndef BIGINT_HPP
#define BIGINT_HPP

#include "common.hpp"

class BigInt {
public:
    bool negative;
    Array<u32> words;
    BigInt();
    BigInt(const BigInt& a);
    BigInt(const BigInt&& a);
    BigInt(const u64& a, bool neg=false);
    BigInt(const u32& a, bool neg=false);
    BigInt(const i64& a);
    BigInt(const i32& a);
    BigInt(const Array<u32>& init, bool neg=false);
    BigInt(const String& string, bool neg=false, const u32 base=10);
    BigInt& operator=(const BigInt& a);
    BigInt& operator=(const u64& a);
    BigInt& operator=(const u32& a);
    BigInt& operator=(const i64& a);
    BigInt& operator=(const i32& a);

    bool operator>(const BigInt& a) const;
    bool operator>=(const BigInt& a) const;
    bool operator<(const BigInt& a) const;
    bool operator<=(const BigInt& a) const;
    bool operator==(const BigInt& a) const;

    bool operator>(const u32& a) const;
    bool operator>=(const u32& a) const;
    bool operator<(const u32& a) const;
    bool operator<=(const u32& a) const;
    bool operator==(const u32& a) const;

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
