/*
	File: Simd.hpp
	Author: Philip Haynes
	Implements SSE2-supported SIMD, with optional improvements from later versions.
*/
#ifndef AZCORE_SIMD_SSE2_HPP
#define AZCORE_SIMD_SSE2_HPP

#include <string.h>

#include <mmintrin.h>  // MMX
#include <xmmintrin.h> // SSE
#include <emmintrin.h> // SSE2
#if __SSE3__
#include <pmmintrin.h>
#endif
#if __SSSE3__
#include <tmmintrin.h>
#endif
#if __SSE4_1__
#include <smmintrin.h>
#endif

#include "../basictypes.hpp"

struct u32x2;
struct i32x2;
struct f64x2;

struct u32x4;
struct i32x4;
struct f32x4;

struct u16x8;
struct i16x8;

inline f32x4 max(f32x4 a, f32x4 b);
inline f32x4 min(f32x4 a, f32x4 b);

inline f64x2 max(f64x2 a, f64x2 b);
inline f64x2 min(f64x2 a, f64x2 b);

#if __SSE4_1__
inline u16x8 max(u16x8 a, u16x8 b);
inline u16x8 min(u16x8 a, u16x8 b);

inline u32x4 max(u32x4 a, u32x4 b);
inline u32x4 min(u32x4 a, u32x4 b);

inline i32x4 max(i32x4 a, i32x4 b);
inline i32x4 min(i32x4 a, i32x4 b);
#endif

inline i16x8 max(i16x8 a, i16x8 b);
inline i16x8 min(i16x8 a, i16x8 b);

struct u16x8 {
	__m128i V;

	u16x8() = default;
	inline force_inline
	u16x8(u16 other) { *this = other; }
	inline force_inline
	u16x8(u16 other[2]) { *this = other; }
	inline force_inline
	u16x8(u16 a, u16 b, u16 c, u16 d, u16 e, u16 f, u16 g, u16 h) {
		V = _mm_setr_epi16(a, b, c, d, e, f, g, h);
	}
	inline force_inline
	u16x8(i16x8 other);
	inline u16x8& force_inline
	operator=(u16 other) {
		V = _mm_set1_epi16(other);
		return *this;
	}
	inline u16x8& force_inline
	operator=(u16 in[8]) {
		V = _mm_setr_epi16(in[0], in[1], in[2], in[3], in[4], in[5], in[6], in[7]);
		return *this;
	}
	// Puts 8 values into dst
	inline void force_inline
	GetValues(u16 *dst) const {
		_mm_storeu_si128((__m128i*)dst, V);
	}
	// Loads 8 values from src
	inline void force_inline
	SetValues(u16 *src) {
		V = _mm_loadu_si128((__m128i*)src);
	}

	template<u32 i>
	inline u16 force_inline
	Get() const {
		u16 result;
		result = _mm_extract_epi16(V, i);
		return result;
	}

	inline u16x8 force_inline
	operator+(u16x8 other) const {
		u16x8 result;
		result.V = _mm_add_epi16(V, other.V);
		return result;
	}
	inline u16x8 force_inline
	operator-(u16x8 other) const {
		u16x8 result;
		result.V = _mm_sub_epi16(V, other.V);
		return result;
	}

	inline u16x8 force_inline
	operator==(u16x8 other) const {
		u16x8 result;
		result.V = _mm_cmpeq_epi16(V, other.V);
		return result;
	}
	inline u16x8 force_inline
	operator!=(u16x8 other) const {
		return ~(*this == other);
	}
	inline u16x8 force_inline
	operator&(u16x8 other) const {
		u16x8 result;
		result.V = _mm_and_si128(V, other.V);
		return result;
	}
	inline u16x8& force_inline
	operator&=(u16x8 other) {
		*this = *this & other;
		return *this;
	}
	inline u16x8 force_inline
	operator|(u16x8 other) const {
		u16x8 result;
		result.V = _mm_or_si128(V, other.V);
		return result;
	}
	inline u16x8& force_inline
	operator|=(u16x8 other) {
		*this = *this | other;
		return *this;
	}
	inline u16x8 force_inline
	operator^(u16x8 other) const {
		u16x8 result;
		result.V = _mm_xor_si128(V, other.V);
		return result;
	}
	inline u16x8& force_inline
	operator^=(u16x8 other) {
		*this = *this ^ other;
		return *this;
	}
	inline u16x8 force_inline
	operator~() const {
		u16x8 result;
		result.V = _mm_xor_si128(V, _mm_set1_epi32(0xFFFFFFFF));
		return result;
	}
};

#if __SSE4_1__
inline u16x8 max(u16x8 a, u16x8 b) {
	u16x8 result;
	result.V = _mm_max_epu16(a.V, b.V);
	return result;
}

inline u16x8 min(u16x8 a, u16x8 b) {
	u16x8 result;
	result.V = _mm_min_epu16(a.V, b.V);
	return result;
}
#endif // __SSE4_1__

inline u16x8 force_inline
AndNot(u16x8 lhs, u16x8 rhs) {
	u16x8 result;
	result.V = _mm_andnot_si128(rhs.V, lhs.V);
	return result;
}



struct i16x8 {
	__m128i V;

	i16x8() = default;
	inline force_inline
	i16x8(i16 other) { *this = other; }
	inline force_inline
	i16x8(i16 other[8]) { *this = other; }
	inline force_inline
	i16x8(i16 a, i16 b, i16 c, i16 d, i16 e, i16 f, i16 g, i16 h) {
		V = _mm_setr_epi16(a, b, c, d, e, f, g, h);
	}
	inline i16x8(u16x8 other) : V(other.V) {}
	inline i16x8& operator=(i16 other) {
		V = _mm_set1_epi16(other);
		return *this;
	}
	inline i16x8& operator=(i16 in[8]) {
		V = _mm_setr_epi16(in[0], in[1], in[2], in[3], in[4], in[5], in[6], in[7]);
		return *this;
	}

	// Puts 8 values into dst
	inline void force_inline
	GetValues(i16 *dst) const {
		_mm_storeu_si128((__m128i*)dst, V);
	}
	// Loads 8 values from src
	inline void force_inline
	SetValues(i16 *src) {
		V = _mm_loadu_si128((const __m128i_u*)src);
	}

	template<u32 i>
	inline i16 force_inline
	Get() const {
		i16 result;
		result = _mm_extract_epi16(V, i);
		return result;
	}

	inline i16x8 force_inline
	operator*(i16x8 other) const {
		i16x8 result;
		result.V = _mm_mullo_epi16(V, other.V);
		return result;
	}
	inline i16x8& force_inline
	operator*=(i16x8 other) {
		*this = *this * other;
		return *this;
	}

	inline i16x8 force_inline
	operator+(i16x8 other) const {
		i16x8 result;
		result.V = _mm_add_epi16(V, other.V);
		return result;
	}
	inline i16x8& force_inline
	operator+=(i16x8 other) {
		*this = *this + other;
		return *this;
	}
	inline i16x8 force_inline
	operator-(i16x8 other) const {
		i16x8 result;
		result.V = _mm_sub_epi16(V, other.V);
		return result;
	}
	inline i16x8& force_inline
	operator-=(i16x8 other) {
		*this = *this - other;
		return *this;
	}
	inline u16x8 force_inline
	operator<(i16x8 other) const {
		u16x8 result;
		result.V = _mm_cmplt_epi16(V, other.V);
		return result;
	}
	inline u16x8 force_inline
	operator>(i16x8 other) const {
		u16x8 result;
		result.V = _mm_cmpgt_epi16(V, other.V);
		return result;
	}
	inline u16x8 force_inline
	operator<=(i16x8 other) const {
		return ~operator>(other);
	}
	inline u16x8 force_inline
	operator>=(i16x8 other) const {
		return ~operator<(other);
	}
	inline u16x8 force_inline
	operator==(i16x8 other) const {
		u16x8 result;
		result.V = _mm_cmpeq_epi16(V, other.V);
		return result;
	}
};

inline force_inline
u16x8::u16x8(i16x8 other) : V(other.V) {}

inline i16x8 force_inline
max(i16x8 a, i16x8 b) {
	i16x8 result;
	result.V = _mm_max_epi16(a.V, b.V);
	return result;
}

inline i16x8 force_inline
min(i16x8 a, i16x8 b) {
	i16x8 result;
	result.V = _mm_min_epi16(a.V, b.V);
	return result;
}



struct u32x2 {
	__m64 V;

	u32x2() = default;
	inline force_inline
	u32x2(u32 other) { *this = other; }
	inline force_inline
	u32x2(u32 other[2]) { *this = other; }
	inline force_inline
	u32x2(u32 a, u32 b) {
		V = _mm_setr_pi32(a, b);
	}
	inline force_inline
	u32x2(i32x2 other);
	inline u32x2& force_inline
	operator=(u32 other) {
		V = _mm_set1_pi32(other);
		return *this;
	}
	inline u32x2& force_inline
	operator=(u32 in[2]) {
		V = _mm_setr_pi32(in[0], in[1]);
		return *this;
	}
	// Puts 2 values into dst
	inline void force_inline
	GetValues(u32 *dst) const {
		dst[0] = Get<0>();
		dst[1] = Get<1>();
	}
	// Loads 2 values from src
	inline void force_inline
	SetValues(u32 *src) {
		*this = src;
	}

	template<u32 i>
	inline u32 force_inline
	Get() const {
		u32 result;
		if constexpr (i == 0) {
			result = _mm_cvtsi64_si32(V);
		} else {
			result = _mm_cvtsi64_si32(_mm_shuffle_pi16(V, _MM_SHUFFLE(1, 0, 3, 2)));
		}
		return result;
	}

	inline u32x2 force_inline
	operator+(u32x2 other) const {
		u32x2 result;
		result.V = _mm_add_pi32(V, other.V);
		return result;
	}
	inline u32x2 force_inline
	operator-(u32x2 other) const {
		u32x2 result;
		result.V = _mm_sub_pi32(V, other.V);
		return result;
	}

	inline u32x2 force_inline
	operator==(u32x2 other) const {
		u32x2 result;
		result.V = _mm_cmpeq_pi32(V, other.V);
		return result;
	}
	inline u32x2 force_inline
	operator!=(u32x2 other) const {
		return ~(*this == other);
	}
	inline u32x2 force_inline
	operator&(u32x2 other) const {
		u32x2 result;
		result.V = _mm_and_si64(V, other.V);
		return result;
	}
	inline u32x2& force_inline
	operator&=(u32x2 other) {
		*this = *this & other;
		return *this;
	}
	inline u32x2 force_inline
	operator|(u32x2 other) const {
		u32x2 result;
		result.V = _mm_or_si64(V, other.V);
		return result;
	}
	inline u32x2& force_inline
	operator|=(u32x2 other) {
		*this = *this | other;
		return *this;
	}
	inline u32x2 force_inline
	operator^(u32x2 other) const {
		u32x2 result;
		result.V = _mm_xor_si64(V, other.V);
		return result;
	}
	inline u32x2& force_inline
	operator^=(u32x2 other) {
		*this = *this ^ other;
		return *this;
	}
	inline u32x2 force_inline
	operator~() const {
		u32x2 result;
		result.V = _mm_xor_si64(V, _mm_set1_pi32(0xFFFFFFFF));
		return result;
	}
};

inline u32x2 force_inline
AndNot(u32x2 lhs, u32x2 rhs) {
	u32x2 result;
	result.V = _mm_andnot_si64(rhs.V, lhs.V);
	return result;
}

inline u32 force_inline
horizontalAdd(u32x2 a) {
	// a.V = 0, 1
	__m64 shuf = _mm_shuffle_pi16(a.V, _MM_SHUFFLE(1, 0, 3, 2));
	// shuf = 1, 0
	__m64 sums = _mm_add_pi32(shuf, a.V);
	// sums = 0+1, 1+0
	return _mm_cvtsi64_si32(sums); // return 0+1
}



struct u32x4 {
	__m128i V;

	u32x4() = default;
	inline force_inline
	u32x4(u32 other) { *this = other; }
	inline force_inline
	u32x4(u32 other[4]) { *this = other; }
	inline force_inline
	u32x4(u32 a, u32 b, u32 c, u32 d) {
		V = _mm_setr_epi32(a, b, c, d);
	}
	inline force_inline
	u32x4(i32x4 other);
	inline u32x4& force_inline
	operator=(u32 other) {
		V = _mm_set1_epi32(other);
		return *this;
	}
	inline u32x4& force_inline
	operator=(u32 in[4]) {
		V = _mm_setr_epi32(in[0], in[1], in[2], in[3]);
		return *this;
	}
	// Puts 4 values into dst
	inline void force_inline
	GetValues(u32 *dst) const {
		_mm_storeu_si128((__m128i*)dst, V);
	}
	// Loads 4 values from src
	inline void force_inline
	SetValues(u32 *src) {
		V = _mm_loadu_si128((const __m128i_u*)src);
	}

	template<u32 i>
	inline u32 force_inline
	Get() const {
		u32 result;
		if constexpr (i == 0) {
			result = _mm_cvtsi128_si32(V);
		} else {
#if __SSE4_1__
			result = _mm_extract_epi32(V, i);
#else
			result = _mm_cvtsi128_si32(_mm_shuffle_epi32(V, _MM_SHUFFLE(i, i, i, i)));
#endif
		}
		return result;
	}

	inline u32x4 force_inline
	operator+(u32x4 other) const {
		u32x4 result;
		result.V = _mm_add_epi32(V, other.V);
		return result;
	}
	inline u32x4 force_inline
	operator-(u32x4 other) const {
		u32x4 result;
		result.V = _mm_sub_epi32(V, other.V);
		return result;
	}

#if __SSE4_1__
	inline u32x4 force_inline
	operator*(u32x4 other) const {
		u32x4 result;
		result.V = _mm_mullo_epi32(V, other.V);
		return result;
	}
	inline u32x4& force_inline
	operator*=(u32x4 other) {
		*this = *this * other;
		return *this;
	}

	inline u32x4 force_inline
	operator>=(u32x4 other) const {
		return max(*this, other) == *this;
	}
	inline u32x4 force_inline
	operator<=(u32x4 other) const {
		return max(*this, other) == other;
	}
	inline u32x4 force_inline
	operator>(u32x4 other) const {
		return ~operator<=(other);
	}
	inline u32x4 force_inline
	operator<(u32x4 other) const {
		return ~operator>=(other);
	}
#endif

	inline u32x4 force_inline
	operator==(u32x4 other) const {
		u32x4 result;
		result.V = _mm_cmpeq_epi32(V, other.V);
		return result;
	}
	inline u32x4 force_inline
	operator!=(u32x4 other) const {
		u32x4 result;
		result.V = _mm_cmpeq_epi32(V, other.V);
		return ~result;
	}
	inline u32x4 force_inline
	operator&(u32x4 other) const {
		u32x4 result;
		result.V = _mm_and_si128(V, other.V);
		return result;
	}
	inline u32x4& force_inline
	operator&=(u32x4 other) {
		*this = *this & other;
		return *this;
	}
	inline u32x4 force_inline
	operator|(u32x4 other) const {
		u32x4 result;
		result.V = _mm_or_si128(V, other.V);
		return result;
	}
	inline u32x4& force_inline
	operator|=(u32x4 other) {
		*this = *this | other;
		return *this;
	}
	inline u32x4 force_inline
	operator^(u32x4 other) const {
		u32x4 result;
		result.V = _mm_xor_si128(V, other.V);
		return result;
	}
	inline u32x4& force_inline
	operator^=(u32x4 other) {
		*this = *this ^ other;
		return *this;
	}
	inline u32x4 force_inline
	operator~() const {
		u32x4 result;
		result.V = _mm_xor_si128(V, _mm_set1_epi32(0xFFFFFFFF));
		return result;
	}
};

#if __SSE4_1__
inline u32x4 force_inline
max(u32x4 a, u32x4 b) {
	u32x4 result;
	result.V = _mm_max_epu32(a.V, b.V);
	return result;
}

inline u32x4 force_inline
min(u32x4 a, u32x4 b) {
	u32x4 result;
	result.V = _mm_min_epu32(a.V, b.V);
	return result;
}
#endif

inline u32x4 force_inline
AndNot(u32x4 lhs, u32x4 rhs) {
	u32x4 result;
	result.V = _mm_andnot_si128(rhs.V, lhs.V);
	return result;
}

inline u32 force_inline
horizontalAdd(u32x4 a) {
	// a.V = 0, 1, 2, 3
	__m128i shuf = _mm_shuffle_epi32(a.V, _MM_SHUFFLE(1, 0, 3, 2));
	// shuf = 2, 3, 0, 1
	__m128i sums = _mm_add_epi32(shuf, a.V);
	// sums = 0+2, 1+3, 2+0, 3+1
	shuf = _mm_shufflelo_epi16(sums, _MM_SHUFFLE(1, 0, 3, 2));
	// shuf = 1+3, 0+2, 2+0, 3+1
	sums = _mm_add_epi32(sums, shuf);
	// sums = 0+1+2+3, 0+1+2+3, 0+0+2+2, 1+1+3+3
	return _mm_cvtsi128_si32(sums); // return 0+1+2+3
}



struct i32x2 {
	__m64 V;

	i32x2() = default;
	inline force_inline
	i32x2(i32 other) { *this = other; }
	inline force_inline
	i32x2(i32 other[2]) { *this = other; }
	inline force_inline
	i32x2(i32 a, i32 b) {
		V = _mm_setr_pi32(a, b);
	}
	inline i32x2(u32x2 other) : V(other.V) {}
	inline i32x2& operator=(i32 other) {
		V = _mm_set1_pi32(other);
		return *this;
	}
	inline i32x2& operator=(i32 in[2]) {
		V = _mm_setr_pi32(in[0], in[1]);
		return *this;
	}

	// Puts 2 values into dst
	inline void force_inline
	GetValues(i32 *dst) const {
		dst[0] = Get<0>();
		dst[1] = Get<1>();
	}
	// Loads 2 values from src
	inline void force_inline
	SetValues(i32 *src) {
		*this = src;
	}

	template<u32 i>
	inline i32 force_inline
	Get() const {
		i32 result;
		if constexpr (i == 0) {
			result = _mm_cvtsi64_si32(V);
		} else {
			result = _mm_cvtsi64_si32(_mm_shuffle_pi16(V, _MM_SHUFFLE(1, 0, 3, 2)));
		}
		return result;
	}

	inline i32x2 force_inline
	operator+(i32x2 other) const {
		i32x2 result;
		result.V = _mm_add_pi32(V, other.V);
		return result;
	}
	inline i32x2& force_inline
	operator+=(i32x2 other) {
		*this = *this + other;
		return *this;
	}
	inline i32x2 force_inline
	operator-(i32x2 other) const {
		i32x2 result;
		result.V = _mm_sub_pi32(V, other.V);
		return result;
	}
	inline i32x2& force_inline
	operator-=(i32x2 other) {
		*this = *this - other;
		return *this;
	}
	inline u32x2 force_inline
	operator<(i32x2 other) const {
		return ~operator>=(other);
	}
	inline u32x2 force_inline
	operator>(i32x2 other) const {
		u32x2 result;
		result.V = _mm_cmpgt_pi32(V, other.V);
		return result;
	}
	inline u32x2 force_inline
	operator<=(i32x2 other) const {
		return ~operator>(other);
	}
	inline u32x2 force_inline
	operator>=(i32x2 other) const {
		return operator>(other) | operator==(other);
	}
	inline u32x2 force_inline
	operator==(i32x2 other) const {
		u32x2 result;
		result.V = _mm_cmpeq_pi32(V, other.V);
		return result;
	}
};

inline force_inline
u32x2::u32x2(i32x2 other) : V(other.V) {}

inline i32 force_inline
horizontalAdd(i32x2 a) {
	return (i32)horizontalAdd(u32x2(a));
}


struct i32x4 {
	__m128i V;

	i32x4() = default;
	inline force_inline
	i32x4(i32 other) { *this = other; }
	inline force_inline
	i32x4(i32 other[4]) { *this = other; }
	inline force_inline
	i32x4(i32 a, i32 b, i32 c, i32 d) {
		V = _mm_setr_epi32(a, b, c, d);
	}
	inline i32x4(u32x4 other) : V(other.V) {}
	inline i32x4& operator=(i32 other) {
		V = _mm_set1_epi32(other);
		return *this;
	}
	inline i32x4& operator=(i32 in[4]) {
		V = _mm_setr_epi32(in[0], in[1], in[2], in[3]);
		return *this;
	}

	// Puts 4 values into dst
	inline void force_inline
	GetValues(i32 *dst) const {
		_mm_storeu_si128((__m128i*)dst, V);
	}
	// Loads 4 values from src
	inline void force_inline
	SetValues(i32 *src) {
		V = _mm_loadu_si128((const __m128i_u*)src);
	}

	template<u32 i>
	inline i32 force_inline
	Get() const {
		i32 result;
		if constexpr (i == 0) {
			result = _mm_cvtsi128_si32(V);
		} else {
#if __SSE4_1__
			result = _mm_extract_epi32(V, i);
#else
			result = _mm_cvtsi128_si32(_mm_shuffle_epi32(V, _MM_SHUFFLE(i, i, i, i)));
#endif
		}
		return result;
	}

#if __SSE4_1__

	inline i32x4 force_inline
	operator*(u32x4 other) const {
		i32x4 result;
		result.V = _mm_mullo_epi32(V, other.V);
		return result;
	}
	inline i32x4& force_inline
	operator*=(i32x4 other) {
		*this = *this * other;
		return *this;
	}

#endif

	inline i32x4 force_inline
	operator+(i32x4 other) const {
		i32x4 result;
		result.V = _mm_add_epi32(V, other.V);
		return result;
	}
	inline i32x4& force_inline
	operator+=(i32x4 other) {
		*this = *this + other;
		return *this;
	}
	inline i32x4 force_inline
	operator-(i32x4 other) const {
		i32x4 result;
		result.V = _mm_sub_epi32(V, other.V);
		return result;
	}
	inline i32x4& force_inline
	operator-=(i32x4 other) {
		*this = *this - other;
		return *this;
	}
	inline u32x4 force_inline
	operator<(i32x4 other) const {
		u32x4 result;
		result.V = _mm_cmplt_epi32(V, other.V);
		return result;
	}
	inline u32x4 force_inline
	operator>(i32x4 other) const {
		u32x4 result;
		result.V = _mm_cmpgt_epi32(V, other.V);
		return result;
	}
	inline u32x4 force_inline
	operator<=(i32x4 other) const {
		return ~operator>(other);
	}
	inline u32x4 force_inline
	operator>=(i32x4 other) const {
		return ~operator<(other);
	}
	inline u32x4 force_inline
	operator==(i32x4 other) const {
		u32x4 result;
		result.V = _mm_cmpeq_epi32(V, other.V);
		return result;
	}
};

inline force_inline
u32x4::u32x4(i32x4 other) : V(other.V) {}

#if __SSE4_1__
inline i32x4 force_inline
max(i32x4 a, i32x4 b) {
	i32x4 result;
	result.V = _mm_max_epi32(a.V, b.V);
	return result;
}

inline i32x4 force_inline
min(i32x4 a, i32x4 b) {
	i32x4 result;
	result.V = _mm_min_epi32(a.V, b.V);
	return result;
}
#endif

inline i32 force_inline
horizontalAdd(i32x4 a) {
	return (i32)horizontalAdd(u32x4(a));
}

inline u32 broadcastBit(u32 a) {
	return a ? 0xffffffff : 0;
}

inline u32x2 _Comparisonf64x2(__m128d c) {
	u32 mask = _mm_movemask_pd(c);
	return u32x2(broadcastBit(mask & 1), broadcastBit(mask & 2));
}

struct f64x2 {
	__m128d V;

	f64x2() = default;
	inline force_inline
	f64x2(f64 other) { *this = other; }
	inline force_inline
	f64x2(i32 other) { *this = other; }
	inline force_inline
	f64x2(i32x2 other) { *this = other; }
	inline force_inline
	f64x2(f64 other[2]) { *this = other; }
	inline force_inline
	f64x2(f32 a, f32 b) {
		V = _mm_setr_pd(a, b);
	}
	inline f64x2& force_inline
	operator=(f64 other) {
		V = _mm_set1_pd(other);
		return *this;
	}
	inline f64x2& force_inline
	operator=(i32 other) {
		V = _mm_set1_pd((f64)other);
		return *this;
	}
	inline f64x2& force_inline
	operator=(i32x2 other) {
		V = _mm_cvtpi32_pd(other.V);
		return *this;
	}
	inline f64x2& force_inline
	operator=(f32 in[2]) {
		V = _mm_setr_pd(in[0], in[1]);
		return *this;
	}

	// Puts 2 values into dst
	inline void force_inline
	GetValues(f64 *dst) const {
		_mm_storeu_pd(dst, V);
	}
	// Loads 2 values from src
	inline void force_inline
	SetValues(f64 *src) {
		V = _mm_loadu_pd(src);
	}

	template<u32 i>
	inline f64 force_inline
	Get() const {
		static_assert(i < 2);
		f32 result;
		if constexpr (i == 0) {
			result = _mm_cvtsd_f64(V);
		} else {
			result = _mm_cvtsd_f64(_mm_shuffle_pd(V, V, _MM_SHUFFLE2(1, 0)));
		}
		return result;
	}

	inline f64x2 force_inline
	operator+(f64x2 other) const {
		f64x2 result;
		result.V = _mm_add_pd(V, other.V);
		return result;
	}
	inline f64x2& force_inline
	operator+=(f64x2 other) {
		*this = *this + other;
		return *this;
	}
	inline f64x2 force_inline
	operator-(f64x2 other) const {
		f64x2 result;
		result.V = _mm_sub_pd(V, other.V);
		return result;
	}
	inline f64x2& force_inline
	operator-=(f64x2 other) {
		*this = *this - other;
		return *this;
	}
	inline f64x2 force_inline
	operator*(f64x2 other) const {
		f64x2 result;
		result.V = _mm_mul_pd(V, other.V);
		return result;
	}
	inline f64x2& force_inline
	operator*=(f64x2 other) {
		*this = *this * other;
		return *this;
	}
	inline f64x2 force_inline
	operator/(f64x2 other) const {
		f64x2 result;
		result.V = _mm_div_pd(V, other.V);
		return result;
	}
	inline f64x2& force_inline
	operator/=(f64x2 other) {
		*this = *this / other;
		return *this;
	}
	inline u32x2 force_inline
	operator<(f64x2 other) const {
		return _Comparisonf64x2(_mm_cmplt_pd(V, other.V));
	}
	inline u32x2 force_inline
	operator<=(f64x2 other) const {
		return _Comparisonf64x2(_mm_cmple_pd(V, other.V));
	}
	inline u32x2 force_inline
	operator>(f64x2 other) const {
		return _Comparisonf64x2(_mm_cmpgt_pd(V, other.V));
	}
	inline u32x2 force_inline
	operator>=(f64x2 other) const {
		return _Comparisonf64x2(_mm_cmpge_pd(V, other.V));
	}
	inline u32x2 force_inline
	operator==(f64x2 other) const {
		return _Comparisonf64x2(_mm_cmpeq_pd(V, other.V));
	}
	inline u32x2 force_inline
	operator!=(f64x2 other) const {
		return _Comparisonf64x2(_mm_cmpneq_pd(V, other.V));
	}

};

inline f64x2 force_inline
sqrt(f64x2 a) {
	f64x2 result;
	result.V = _mm_sqrt_pd(a.V);
	return result;
}

inline f64x2 force_inline
max(f64x2 a, f64x2 b) {
	f64x2 result;
	result.V = _mm_max_pd(a.V, b.V);
	return result;
}

inline f64x2 force_inline
min(f64x2 a, f64x2 b) {
	f64x2 result;
	result.V = _mm_min_pd(a.V, b.V);
	return result;
}



struct f32x4 {
	__m128 V;

	f32x4() = default;
	inline force_inline
	f32x4(f32 other) { *this = other; }
	inline force_inline
	f32x4(i32 other) { *this = other; }
	inline force_inline
	f32x4(i32x4 other) { *this = other; }
	inline force_inline
	f32x4(f32 other[4]) { *this = other; }
	inline force_inline
	f32x4(f32 a, f32 b, f32 c, f32 d) {
		V = _mm_setr_ps(a, b, c, d);
	}
	inline f32x4& force_inline
	operator=(f32 other) {
		V = _mm_set1_ps(other);
		return *this;
	}
	inline f32x4& force_inline
	operator=(i32 other) {
		V = _mm_set1_ps((f32)other);
		return *this;
	}
	inline f32x4& force_inline
	operator=(i32x4 other) {
		V = _mm_cvtepi32_ps(other.V);
		return *this;
	}
	inline f32x4& force_inline
	operator=(f32 in[4]) {
		V = _mm_setr_ps(in[0], in[1], in[2], in[3]);
		return *this;
	}

	// Puts 4 values into dst
	inline void force_inline
	GetValues(f32 *dst) const {
		_mm_storeu_ps(dst, V);
	}
	// Loads 4 values from src
	inline void force_inline
	SetValues(f32 *src) {
		V = _mm_loadu_ps(src);
	}

	template<u32 i>
	inline f32 force_inline
	Get() const {
		f32 result;
		if constexpr (i == 0) {
			result = _mm_cvtss_f32(V);
		} else {
			result = _mm_cvtss_f32(_mm_shuffle_ps(V, V, _MM_SHUFFLE(i, i, i, i)));
		}
		return result;
	}

	inline f32x4 force_inline
	operator+(f32x4 other) const {
		f32x4 result;
		result.V = _mm_add_ps(V, other.V);
		return result;
	}
	inline f32x4& force_inline
	operator+=(f32x4 other) {
		*this = *this + other;
		return *this;
	}
	inline f32x4 force_inline
	operator-(f32x4 other) const {
		f32x4 result;
		result.V = _mm_sub_ps(V, other.V);
		return result;
	}
	inline f32x4& force_inline
	operator-=(f32x4 other) {
		*this = *this - other;
		return *this;
	}
	inline f32x4 force_inline
	operator*(f32x4 other) const {
		f32x4 result;
		result.V = _mm_mul_ps(V, other.V);
		return result;
	}
	inline f32x4& force_inline
	operator*=(f32x4 other) {
		*this = *this * other;
		return *this;
	}
	inline f32x4 force_inline
	operator/(f32x4 other) const {
		f32x4 result;
		result.V = _mm_div_ps(V, other.V);
		return result;
	}
	inline f32x4& force_inline
	operator/=(f32x4 other) {
		*this = *this / other;
		return *this;
	}
	inline u32x4 force_inline
	operator<(f32x4 other) const {
		u32x4 result;
		result.V = _mm_castps_si128(_mm_cmplt_ps(V, other.V));
		return result;
	}
	inline u32x4 force_inline
	operator<=(f32x4 other) const {
		u32x4 result;
		result.V = _mm_castps_si128(_mm_cmple_ps(V, other.V));
		return result;
	}
	inline u32x4 force_inline
	operator>(f32x4 other) const {
		u32x4 result;
		result.V = _mm_castps_si128(_mm_cmpgt_ps(V, other.V));
		return result;
	}
	inline u32x4 force_inline
	operator>=(f32x4 other) const {
		u32x4 result;
		result.V = _mm_castps_si128(_mm_cmpge_ps(V, other.V));
		return result;
	}
	inline u32x4 force_inline
	operator==(f32x4 other) const {
		u32x4 result;
		result.V = _mm_castps_si128(_mm_cmpeq_ps(V, other.V));
		return result;
	}
	inline u32x4 force_inline
	operator!=(f32x4 other) const {
		u32x4 result;
		result.V = _mm_castps_si128(_mm_cmpneq_ps(V, other.V));
		return result;
	}

};

inline f32x4 force_inline
sqrt(f32x4 a) {
	f32x4 result;
	result.V = _mm_sqrt_ps(a.V);
	return result;
}

inline f32x4 force_inline
rsqrt(f32x4 a) {
	f32x4 result;
	result.V = _mm_rsqrt_ps(a.V);
	return result;
}

inline f32x4 force_inline
reciprocal(f32x4 a) {
	f32x4 result;
	result.V = _mm_rcp_ps(a.V);
	return result;
}

inline f32x4 force_inline
max(f32x4 a, f32x4 b) {
	f32x4 result;
	result.V = _mm_max_ps(a.V, b.V);
	return result;
}

inline f32x4 force_inline
min(f32x4 a, f32x4 b) {
	f32x4 result;
	result.V = _mm_min_ps(a.V, b.V);
	return result;
}

#if __SSE3__
inline f32 force_inline
horizontalAdd(f32x4 a) {
	// a.V = 0, 1, 2, 3
	__m128 shuf = _mm_movehdup_ps(a.V); // shuf = 1,   1,   3,   3
	__m128 sums = _mm_add_ps(a.V, shuf);// sums = 0+1, 1+1, 2+3, 3+3
	shuf = _mm_movehl_ps(shuf, sums);   // shuf = 2+3, 3+3, 3,   3
	sums = _mm_add_ss(sums, shuf);      // sums = 0+1+2+3, 1+1, 2+3, 3+3
	return _mm_cvtss_f32(sums);         // return 0+1+2+3
}
#else
inline f32 force_inline
horizontalAdd(f32x4 a) {
	// a.V = 0, 1, 2, 3
	__m128 shuf = _mm_shuffle_ps(a.V, a.V, _MM_SHUFFLE(2, 3, 0, 1));
	// shuf = 1, 0, 3, 2
	__m128 sums = _mm_add_ps(shuf, sums);
	// sums = 0+1, 0+1, 2+3, 2+3
	shuf = _mm_movehl_ps(shuf, sums);
	// shuf = 2+3, 2+3, 3, 2
	sums = _mm_add_ss(sums, shuf);
	// shuf = 0+1+2+3, 0+1, 2+3, 2+3
	return _mm_cvtss_f32(shuf); // return 0+1+2+3
}
#endif

#endif // AZCORE_SIMD_SSE2_HPP
