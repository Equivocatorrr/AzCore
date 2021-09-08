/*
	File: BigInt.hpp
	Author: Philip Haynes
	An arbitrary-precision integer class that uses a string of
	64-bit integers to represent an integer with any number of digits.
*/
#ifndef AZCORE_BIGINT_HPP
#define AZCORE_BIGINT_HPP

#ifndef BIGINT_BUCKET_SIZE
#define BIGINT_BUCKET_SIZE 15
#endif

#include "BucketArray.hpp"
#include "String.hpp"

namespace AzCore {

#pragma pack(1)

class BigInt {
public:
	BucketArray<u64, BIGINT_BUCKET_SIZE> words;
	static_assert(sizeof(words) == 124);
	u32 negative;
	inline BigInt() : words(1, 0), negative(false) {}
	inline BigInt(const BigInt& a) : words(a.words), negative(a.negative) {}
	explicit inline BigInt(const u128& a, bool neg=false) { *this = a; negative = neg; }
	explicit inline BigInt(u64 a, bool neg=false) { *this = a; negative = neg; }
	explicit inline BigInt(u32 a, bool neg=false) { *this = a; negative = neg; }
	explicit inline BigInt(i64 a) { *this = a; }
	explicit inline BigInt(i32 a) { *this = a; }
	explicit inline BigInt(const BucketArray<u64, BIGINT_BUCKET_SIZE>& init, bool neg=false) : words(init), negative(neg) {}
	explicit BigInt(const String& string, bool neg=false, const u32 base=10);
	inline BigInt& operator=(const BigInt& a) {
		words = a.words;
		negative = a.negative;
		return *this;
	}
	inline BigInt& operator=(const u128& a) {
		u64 a1 = a;
		u64 a2 = a >> 64;
		if (a2) {
			words = {a1, a2};
		} else {
			words = {a1};
		}
		return *this;
	}
	inline BigInt& operator=(u64 a) {
		words = {a};
		return *this;
	}
	inline BigInt& operator=(u32 a) {
		words = {a};
		return *this;
	}
	inline BigInt& operator=(const i128& a) {
		u64 a1 = a;
		u64 a2 = (*((u64*)(&a)+1)) & 0x7fffffffffffffff;
		if (a2) {
			words = {a1, a2};
		} else {
			words = {a1};
		}
		negative = a < 0;
		return *this;
	}
	inline BigInt& operator=(i64 a) {
		u64 aa = (*((u64*)(&a))) & 0x7fffffffffffffff;
		words = {aa};
		negative = a < 0;
		return *this;
	}
	inline BigInt& operator=(i32 a) {
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

	bool operator>(u64 a) const;
	bool operator>=(u64 a) const;
	bool operator<(u64 a) const;
	bool operator<=(u64 a) const;
	inline bool operator==(u64 a) const {
		return words.size == 1 && negative == false && words[0] == a;
	}

	inline bool operator!=(const BigInt& a) const {
		return !(*this == a);
	}

	inline bool operator!=(u64 a) const {
		return !(*this == a);
	}

	BigInt operator-() const;

	BigInt& operator+=(const BigInt& a);
	BigInt& operator-=(const BigInt& a);
	BigInt& operator*=(const BigInt& a);
	BigInt& operator/=(const BigInt& a);
	BigInt& operator%=(const BigInt& a);

	BigInt& operator+=(u64 a);
	BigInt& operator-=(u64 a);
	BigInt& operator*=(u64 a);
	BigInt& operator/=(u64 a);
	BigInt& operator%=(u64 a);

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

	inline BigInt operator+(u64 a) const {
		BigInt temp(*this);
		temp += a;
		return temp;
	}
	inline BigInt operator-(u64 a) const {
		BigInt temp(*this);
		temp -= a;
		return temp;
	}
	inline BigInt operator*(u64 a) const {
		BigInt temp(*this);
		temp *= a;
		return temp;
	}
	inline BigInt operator/(u64 a) const {
		BigInt temp(*this);
		temp /= a;
		return temp;
	}
	inline BigInt operator%(u64 a) const {
		BigInt temp(*this);
		temp %= a;
		return temp;
	}

	static void QuotientAndRemainder(const BigInt& a, const BigInt& b, BigInt *dstQuotient, BigInt *dstRemainder);
	static void QuotientAndRemainder(const BigInt& a, u64 b, BigInt *dstQuotient, u64 *dstRemainder);

	BigInt& operator<<=(i32 i);
	BigInt& operator>>=(i32 i);

	inline BigInt operator<<(i32 i) const {
		BigInt temp(*this);
		temp <<= i;
		return temp;
	}
	inline BigInt operator>>(i32 i) const {
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

} // namespace AzCore

#endif
