/*
	File: Simd.hpp
	Author: Philip Haynes
	Implements SSE2-supported SIMD, with optional improvements from later versions.
*/
#ifndef AZCORE_SIMD_SSE2_HPP
#define AZCORE_SIMD_SSE2_HPP

#include <mmintrin.h>  // MMX NOTE: These are deprecated in MSVC
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
#if __SSE4_2__
#include <nmmintrin.h>
#endif

#include "../basictypes.hpp"

// Represents all integer types.
// Defines bitwise operations that don't depend on lane width
struct _si128;

// Inherits from _si128
// Defines integer operations for 2 64-bit integers that don't depend on sign
struct _int64x2;

struct u64x2;
struct i64x2;
struct f64x2;

// Inherits from _si128
// Defines integer operations for 4 32-bit integers that don't depend on sign
struct _int32x4;

struct u32x4;
struct i32x4;
struct f32x4;

// Inherits from _si128
// Defines integer operations for 8 16-bit integers that don't depend on sign
struct _int16x8;

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

#if __SSE4_2__
inline i64x2 max(i64x2 a, i64x2 b);
inline i64x2 min(i64x2 a, i64x2 b);
#endif

inline i16x8 max(i16x8 a, i16x8 b);
inline i16x8 min(i16x8 a, i16x8 b);

struct _si128 {
	__m128i V;

	force_inline(void)
	GetValues(char *dst) const {
		_mm_storeu_si128((__m128i*)dst, V);
	}
	force_inline(void)
	SetValues(const char *src) {
		V = _mm_loadu_si128((__m128i*)src);
	}

	force_inline(_si128)
	operator&(_si128 other) const {
		_si128 result;
		result.V = _mm_and_si128(V, other.V);
		return result;
	}
	force_inline(_si128&)
	operator&=(_si128 other) {
		*this = *this & other;
		return *this;
	}
	force_inline(_si128)
	operator|(_si128 other) const {
		_si128 result;
		result.V = _mm_or_si128(V, other.V);
		return result;
	}
	force_inline(_si128&)
	operator|=(_si128 other) {
		*this = *this | other;
		return *this;
	}
	force_inline(_si128)
	operator^(_si128 other) const {
		_si128 result;
		result.V = _mm_xor_si128(V, other.V);
		return result;
	}
	force_inline(_si128&)
	operator^=(_si128 other) {
		*this = *this ^ other;
		return *this;
	}
	force_inline(_si128)
	operator~() const {
		_si128 result;
		result.V = _mm_xor_si128(V, _mm_set1_epi32(0xFFFFFFFF));
		return result;
	}
};

force_inline(_si128)
AndNot(_si128 lhs, _si128 rhs) {
	_si128 result;
	result.V = _mm_andnot_si128(rhs.V, lhs.V);
	return result;
}

struct _int16x8 : public _si128 {
	_int16x8() = default;
	force_inline()
	_int16x8(_si128 other) { V = other.V; }
	force_inline(_int16x8&)
	operator=(u16 other) {
		V = _mm_set1_epi16(other);
		return *this;
	}
	force_inline(_int16x8&)
	operator=(u16 in[8]) {
		V = _mm_setr_epi16(in[0], in[1], in[2], in[3], in[4], in[5], in[6], in[7]);
		return *this;
	}
	force_inline(_int16x8&)
	operator=(i16 other) {
		return operator=((u16)other);
	}
	force_inline(_int16x8&)
	operator=(i16 in[8]) {
		return operator=((u16*)in);
	}

	template<u32 i>
	force_inline(u16)
	Get() const {
		u16 result;
		result = _mm_extract_epi16(V, i);
		return result;
	}

	force_inline(_int16x8)
	operator+(_int16x8 other) const {
		_int16x8 result;
		result.V = _mm_add_epi16(V, other.V);
		return result;
	}
	force_inline(_int16x8)
	operator-(_int16x8 other) const {
		_int16x8 result;
		result.V = _mm_sub_epi16(V, other.V);
		return result;
	}

	force_inline(_int16x8)
	operator==(_int16x8 other) const {
		_int16x8 result;
		result.V = _mm_cmpeq_epi16(V, other.V);
		return result;
	}
	force_inline(_int16x8)
	operator!=(_int16x8 other) const {
		return ~operator==(other);
	}
};

struct u16x8 : public _int16x8 {
	u16x8() = default;
	force_inline()
	u16x8(u16 other) { *this = other; }
	force_inline()
	u16x8(u16 other[2]) { *this = other; }
	force_inline()
	u16x8(u16 a, u16 b, u16 c, u16 d, u16 e, u16 f, u16 g, u16 h) {
		V = _mm_setr_epi16(a, b, c, d, e, f, g, h);
	}
	force_inline()
	u16x8(i16x8 other);
	force_inline()
	u16x8(_si128 other) { V = other.V; }
	force_inline()
	u16x8(_int16x8 other) { V = other.V; }
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



struct i16x8 : public _int16x8 {
	i16x8() = default;
	force_inline()
	i16x8(i16 other) { *this = other; }
	force_inline()
	i16x8(i16 other[8]) { *this = other; }
	force_inline()
	i16x8(i16 a, i16 b, i16 c, i16 d, i16 e, i16 f, i16 g, i16 h) {
		V = _mm_setr_epi16(a, b, c, d, e, f, g, h);
	}
	force_inline()
	i16x8(u16x8 other) { V = other.V; }
	force_inline()
	i16x8(_si128 other) { V = other.V; }
	force_inline()
	i16x8(_int16x8 other) { V = other.V; }

	template<u32 i>
	force_inline(i16)
	Get() const {
		return (i16)_int16x8::Get<i>();
	}

	force_inline(i16x8)
	operator*(i16x8 other) const {
		i16x8 result;
		result.V = _mm_mullo_epi16(V, other.V);
		return result;
	}
	force_inline(i16x8&)
	operator*=(i16x8 other) {
		*this = *this * other;
		return *this;
	}

	force_inline(u16x8)
	operator<(i16x8 other) const {
		u16x8 result;
		result.V = _mm_cmplt_epi16(V, other.V);
		return result;
	}
	force_inline(u16x8)
	operator>(i16x8 other) const {
		u16x8 result;
		result.V = _mm_cmpgt_epi16(V, other.V);
		return result;
	}
	force_inline(u16x8)
	operator<=(i16x8 other) const {
		return ~operator>(other);
	}
	force_inline(u16x8)
	operator>=(i16x8 other) const {
		return ~operator<(other);
	}
};

force_inline()
u16x8::u16x8(i16x8 other) { V = other.V; }

force_inline(i16x8)
max(i16x8 a, i16x8 b) {
	i16x8 result;
	result.V = _mm_max_epi16(a.V, b.V);
	return result;
}

force_inline(i16x8)
min(i16x8 a, i16x8 b) {
	i16x8 result;
	result.V = _mm_min_epi16(a.V, b.V);
	return result;
}



struct _int32x4 : public _si128 {
	force_inline(_int32x4&)
	operator=(u32 other) {
		V = _mm_set1_epi32(other);
		return *this;
	}
	force_inline(_int32x4&)
	operator=(u32 in[4]) {
		V = _mm_setr_epi32(in[0], in[1], in[2], in[3]);
		return *this;
	}
	force_inline(_int32x4&)
	operator=(i32 other) {
		return operator=((u32)other);
	}
	force_inline(_int32x4&)
	operator=(i32 in[4]) {
		return operator=((u32*)in);
	}
	force_inline(void)
	GetValues(u32 *dst) const {
		_mm_storeu_si128((__m128i*)dst, V);
	}
	force_inline(void)
	SetValues(const u32 *src) {
		V = _mm_loadu_si128((const __m128i*)src);
	}
	force_inline(void)
	GetValues(i32 *dst) const {
		_mm_storeu_si128((__m128i*)dst, V);
	}
	force_inline(void)
	SetValues(const i32 *src) {
		V = _mm_loadu_si128((const __m128i*)src);
	}

	template<u32 i>
	force_inline(u32)
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

	force_inline(_int32x4)
	operator+(_int32x4 other) const {
		_int32x4 result;
		result.V = _mm_add_epi32(V, other.V);
		return result;
	}
	force_inline(_int32x4&)
	operator+=(_int32x4 other) {
		V = _mm_add_epi32(V, other.V);
		return *this;
	}
	force_inline(_int32x4)
	operator-(_int32x4 other) const {
		_int32x4 result;
		result.V = _mm_sub_epi32(V, other.V);
		return result;
	}
	force_inline(_int32x4&)
	operator-=(_int32x4 other) {
		V = _mm_sub_epi32(V, other.V);
		return *this;
	}

	force_inline(_int32x4)
	operator==(_int32x4 other) const {
		_int32x4 result;
		result.V = _mm_cmpeq_epi32(V, other.V);
		return result;
	}
	force_inline(_int32x4)
	operator!=(_int32x4 other) const {
		_int32x4 result;
		result.V = _mm_cmpeq_epi32(V, other.V);
		return ~result;
	}
	force_inline(_int32x4)
	operator&(_int32x4 other) const {
		_int32x4 result;
		result.V = _mm_and_si128(V, other.V);
		return result;
	}
	force_inline(_int32x4&)
	operator&=(_int32x4 other) {
		*this = *this & other;
		return *this;
	}
	force_inline(_int32x4)
	operator|(_int32x4 other) const {
		_int32x4 result;
		result.V = _mm_or_si128(V, other.V);
		return result;
	}
	force_inline(_int32x4&)
	operator|=(_int32x4 other) {
		*this = *this | other;
		return *this;
	}
	force_inline(_int32x4)
	operator^(_int32x4 other) const {
		_int32x4 result;
		result.V = _mm_xor_si128(V, other.V);
		return result;
	}
	force_inline(_int32x4&)
	operator^=(_int32x4 other) {
		*this = *this ^ other;
		return *this;
	}
	force_inline(_int32x4)
	operator~() const {
		_int32x4 result;
		result.V = _mm_xor_si128(V, _mm_set1_epi32(0xFFFFFFFF));
		return result;
	}
};



struct u32x4 : public _int32x4 {
	u32x4() = default;
	force_inline()
	u32x4(u32 other) { _int32x4::operator=(other); }
	force_inline()
	u32x4(u32 other[4]) { _int32x4::operator=(other); }
	force_inline()
	u32x4(u32 a, u32 b, u32 c, u32 d) {
		V = _mm_setr_epi32(a, b, c, d);
	}
	force_inline()
	u32x4(i32x4 other);
	force_inline()
	u32x4(_si128 other) { V = other.V; }
	force_inline()
	u32x4(_int32x4 other) { V = other.V; }
};

#if __SSE4_1__
force_inline(u32x4)
max(u32x4 a, u32x4 b) {
	u32x4 result;
	result.V = _mm_max_epu32(a.V, b.V);
	return result;
}

force_inline(u32x4)
min(u32x4 a, u32x4 b) {
	u32x4 result;
	result.V = _mm_min_epu32(a.V, b.V);
	return result;
}
#endif

force_inline(u32)
horizontalAdd(_int32x4 a) {
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



struct i32x4 : public _int32x4 {
	i32x4() = default;
	force_inline()
	i32x4(i32 other) { _int32x4::operator=(other); }
	force_inline()
	i32x4(i32 other[4]) { _int32x4::operator=(other); }
	force_inline()
	i32x4(i32 a, i32 b, i32 c, i32 d) {
		V = _mm_setr_epi32(a, b, c, d);
	}
	force_inline()
	i32x4(u32x4 other) { V = other.V; }
	force_inline()
	i32x4(_si128 other) { V = other.V; }
	force_inline()
	i32x4(_int32x4 other) { V = other.V; }

	template<u32 i>
	force_inline(i32)
	Get() const {
		return (i32)_int32x4::Get<i>();
	}

#if __SSE4_1__

	force_inline(i32x4)
	operator*(u32x4 other) const {
		i32x4 result;
		result.V = _mm_mullo_epi32(V, other.V);
		return result;
	}
	force_inline(i32x4&)
	operator*=(i32x4 other) {
		*this = *this * other;
		return *this;
	}

#endif

	force_inline(u32x4)
	operator<(i32x4 other) const {
		u32x4 result;
		result.V = _mm_cmplt_epi32(V, other.V);
		return result;
	}
	force_inline(u32x4)
	operator>(i32x4 other) const {
		u32x4 result;
		result.V = _mm_cmpgt_epi32(V, other.V);
		return result;
	}
	force_inline(u32x4)
	operator<=(i32x4 other) const {
		return ~operator>(other);
	}
	force_inline(u32x4)
	operator>=(i32x4 other) const {
		return ~operator<(other);
	}
};

force_inline()
u32x4::u32x4(i32x4 other) { V = other.V; }

#if __SSE4_1__
force_inline(i32x4)
max(i32x4 a, i32x4 b) {
	i32x4 result;
	result.V = _mm_max_epi32(a.V, b.V);
	return result;
}

force_inline(i32x4)
min(i32x4 a, i32x4 b) {
	i32x4 result;
	result.V = _mm_min_epi32(a.V, b.V);
	return result;
}
#endif

force_inline(i32)
horizontalAdd(i32x4 a) {
	return (i32)horizontalAdd(u32x4(a));
}



struct _int64x2 : public _si128 {
	_int64x2() = default;
	force_inline()
	_int64x2(_si128 other) { V = other.V; }
	force_inline(_int64x2&)
	operator=(u64 other) {
		V = _mm_set1_epi64x(other);
		return *this;
	}
	force_inline(_int64x2&)
	operator=(u64 in[2]) {
		V = _mm_set_epi64x(in[0], in[1]);
		return *this;
	}
	force_inline(_int64x2&)
	operator=(i64 other) {
		return operator=((u64)other);
	}
	force_inline(_int64x2&)
	operator=(i64 in[2]) {
		return operator=((u64*)in);
	}
	force_inline(void)
	GetValues(u64 *dst) const {
		_mm_storeu_si128((__m128i*)dst, V);
	}
	force_inline(void)
	SetValues(const u64 *src) {
		V = _mm_loadu_si128((const __m128i*)src);
	}
	force_inline(void)
	GetValues(i64 *dst) const {
		_mm_storeu_si128((__m128i*)dst, V);
	}
	force_inline(void)
	SetValues(const i64 *src) {
		V = _mm_loadu_si128((const __m128i*)src);
	}

	force_inline(_int64x2)
	operator+(_int64x2 other) const {
		_int64x2 result;
		result.V = _mm_add_epi64(V, other.V);
		return result;
	}
	force_inline(_int64x2&)
	operator+=(_int64x2 other) {
		V = _mm_add_epi64(V, other.V);
		return *this;
	}
	force_inline(_int64x2)
	operator-(_int64x2 other) const {
		_int64x2 result;
		result.V = _mm_sub_epi64(V, other.V);
		return result;
	}
	force_inline(_int64x2&)
	operator-=(_int64x2 other) {
		V = _mm_sub_epi64(V, other.V);
		return *this;
	}

#if __SSE4_1__
	force_inline(_int64x2)
	operator==(_int64x2 other) const {
		_int64x2 result;
		result.V = _mm_cmpeq_epi64(V, other.V);
		return result;
	}
	force_inline(_int64x2)
	operator!=(_int64x2 other) const {
		_int64x2 result;
		result.V = _mm_cmpeq_epi64(V, other.V);
		return ~result;
	}
#endif

	template<u32 i>
	force_inline(u64)
	Get() const {
		u64 result;
		if constexpr (i == 0) {
			result = _mm_cvtsi128_si64(V);
		} else {
#if __SSE4_1__
			result = _mm_extract_epi64(V, i);
#else
			result = _mm_cvtsi128_si64(_mm_unpackhi_epi64(V,V));
#endif
		}
		return result;
	}
};



struct u64x2 : public _int64x2 {
	u64x2() = default;
	force_inline()
	u64x2(u64 other) { *this = other; }
	force_inline()
	u64x2(u64 other[2]) { *this = other; }
	force_inline()
	u64x2(u64 a, u64 b) {
		V = _mm_set_epi64x(a, b);
	}
	force_inline()
	u64x2(i64x2 other);
	force_inline()
	u64x2(_si128 other) { V = other.V; }
	force_inline()
	u64x2(_int64x2 other) { V = other.V; }
};

force_inline(u64)
horizontalAdd(u64x2 a) {
	// a.V = 0, 1
	__m128i shuf = _mm_unpackhi_epi64(a.V, a.V);
	// shuf = 1, 1
	__m128i sums = _mm_add_epi64(shuf, a.V);
	// sums = 0+1, 1+1
	return _mm_cvtsi128_si64(sums); // return 0+1
}



struct i64x2 : public _int64x2 {
	i64x2() = default;
	force_inline()
	i64x2(i64 other) { *this = other; }
	force_inline()
	i64x2(i64 other[2]) { *this = other; }
	force_inline()
	i64x2(i64 a, i64 b) {
		V = _mm_set_epi64x(a, b);
	}
	force_inline()
	i64x2(u64x2 other) { V = other.V; }
	force_inline()
	i64x2(_si128 other) { V = other.V; }
	force_inline()
	i64x2(_int64x2 other) { V = other.V; }

	template<u32 i>
	force_inline(i64)
	Get() const {
		return (i64)_int64x2::Get<i>();
	}

#if __SSE4_2__
	force_inline(i64x2)
	operator>(i64x2 other) const {
		u64x2 result;
		result.V = _mm_cmpgt_epi64(V, other.V);
		return result;
	}
	force_inline(i64x2)
	operator<(i64x2 other) const {
		return other.operator>(*this);
	}
	force_inline(i64x2)
	operator>=(i64x2 other) const {
		return ~operator<(*this);
	}
	force_inline(i64x2)
	operator<=(i64x2 other) const {
		return ~operator>(*this);
	}
#endif
};

force_inline(i64)
horizontalAdd(i64x2 a) {
	return (i64)horizontalAdd(u64x2(a));
}

force_inline()
u64x2::u64x2(i64x2 other) {
	V = other.V;
}

#if __SSE4_2__
force_inline(i64x2)
max(i64x2 a, i64x2 b) {
	i64x2 mask = a > b;
	return (a & mask) | AndNot(b, mask);
}

force_inline(i64x2)
min(i64x2 a, i64x2 b) {
	i64x2 mask = b > a;
	return (a & mask) | AndNot(b, mask);
}
#endif



inline u32 broadcastBit(u32 a) {
	return a ? 0xffffffff : 0;
}

inline u32x4 _Comparisonf64x2(__m128d c) {
	u64 mask = _mm_movemask_pd(c);
	return u32x4(broadcastBit(mask & 1), broadcastBit(mask & 2), 0, 0);
}

struct f64x2 {
	__m128d V;

	f64x2() = default;
	force_inline()
	f64x2(f64 other) { *this = other; }
	force_inline()
	f64x2(i32 other) { *this = other; }
	// Copies and converts the lower 2 elements of i32x4
	force_inline()
	f64x2(i32x4 other) { *this = other; }
	force_inline()
	f64x2(f64 other[2]) { *this = other; }
	force_inline()
	f64x2(f64 a, f64 b) {
		V = _mm_setr_pd(a, b);
	}
	force_inline(f64x2&)
	operator=(f64 other) {
		V = _mm_set1_pd(other);
		return *this;
	}
	// Copies and converts the lower 2 elements of i32x4
	force_inline(f64x2&)
	operator=(i32x4 other) {
		V = _mm_cvtepi32_pd(other.V);
		return *this;
	}
	force_inline(f64x2&)
	operator=(f64 in[2]) {
		V = _mm_setr_pd(in[0], in[1]);
		return *this;
	}

	// Puts 2 values into dst
	force_inline(void)
	GetValues(f64 *dst) const {
		_mm_storeu_pd(dst, V);
	}
	// Loads 2 values from src
	force_inline(void)
	SetValues(const f64 *src) {
		V = _mm_loadu_pd(src);
	}

	template<u32 i>
	force_inline(f64)
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

	force_inline(f64x2)
	operator+(f64x2 other) const {
		f64x2 result;
		result.V = _mm_add_pd(V, other.V);
		return result;
	}
	force_inline(f64x2&)
	operator+=(f64x2 other) {
		*this = *this + other;
		return *this;
	}
	force_inline(f64x2)
	operator-(f64x2 other) const {
		f64x2 result;
		result.V = _mm_sub_pd(V, other.V);
		return result;
	}
	force_inline(f64x2&)
	operator-=(f64x2 other) {
		*this = *this - other;
		return *this;
	}
	force_inline(f64x2)
	operator*(f64x2 other) const {
		f64x2 result;
		result.V = _mm_mul_pd(V, other.V);
		return result;
	}
	force_inline(f64x2&)
	operator*=(f64x2 other) {
		*this = *this * other;
		return *this;
	}
	force_inline(f64x2)
	operator/(f64x2 other) const {
		f64x2 result;
		result.V = _mm_div_pd(V, other.V);
		return result;
	}
	force_inline(f64x2&)
	operator/=(f64x2 other) {
		*this = *this / other;
		return *this;
	}
	force_inline(u32x4)
	operator<(f64x2 other) const {
		return _Comparisonf64x2(_mm_cmplt_pd(V, other.V));
	}
	force_inline(u32x4)
	operator<=(f64x2 other) const {
		return _Comparisonf64x2(_mm_cmple_pd(V, other.V));
	}
	force_inline(u32x4)
	operator>(f64x2 other) const {
		return _Comparisonf64x2(_mm_cmpgt_pd(V, other.V));
	}
	force_inline(u32x4)
	operator>=(f64x2 other) const {
		return _Comparisonf64x2(_mm_cmpge_pd(V, other.V));
	}
	force_inline(u32x4)
	operator==(f64x2 other) const {
		return _Comparisonf64x2(_mm_cmpeq_pd(V, other.V));
	}
	force_inline(u32x4)
	operator!=(f64x2 other) const {
		return _Comparisonf64x2(_mm_cmpneq_pd(V, other.V));
	}

};

force_inline(f64x2)
sqrt(f64x2 a) {
	f64x2 result;
	result.V = _mm_sqrt_pd(a.V);
	return result;
}

force_inline(f64x2)
max(f64x2 a, f64x2 b) {
	f64x2 result;
	result.V = _mm_max_pd(a.V, b.V);
	return result;
}

force_inline(f64x2)
min(f64x2 a, f64x2 b) {
	f64x2 result;
	result.V = _mm_min_pd(a.V, b.V);
	return result;
}



struct f32x4 {
	__m128 V;

	f32x4() = default;
	force_inline()
	f32x4(f32 other) { *this = other; }
	force_inline()
	f32x4(i32 other) { *this = other; }
	force_inline()
	f32x4(i32x4 other) { *this = other; }
	force_inline()
	f32x4(f32 other[4]) { *this = other; }
	force_inline()
	f32x4(f32 a, f32 b, f32 c, f32 d) {
		V = _mm_setr_ps(a, b, c, d);
	}
	force_inline(f32x4&)
	operator=(f32 other) {
		V = _mm_set1_ps(other);
		return *this;
	}
	force_inline(f32x4&)
	operator=(i32 other) {
		V = _mm_set1_ps((f32)other);
		return *this;
	}
	force_inline(f32x4&)
	operator=(i32x4 other) {
		V = _mm_cvtepi32_ps(other.V);
		return *this;
	}
	force_inline(f32x4&)
	operator=(f32 in[4]) {
		V = _mm_setr_ps(in[0], in[1], in[2], in[3]);
		return *this;
	}

	// Puts 4 values into dst
	force_inline(void)
	GetValues(f32 *dst) const {
		_mm_storeu_ps(dst, V);
	}
	// Loads 4 values from src
	force_inline(void)
	SetValues(const f32 *src) {
		V = _mm_loadu_ps(src);
	}

	template<u32 i>
	force_inline(f32)
	Get() const {
		f32 result;
		if constexpr (i == 0) {
			result = _mm_cvtss_f32(V);
		} else {
			result = _mm_cvtss_f32(_mm_shuffle_ps(V, V, _MM_SHUFFLE(i, i, i, i)));
		}
		return result;
	}

	force_inline(f32x4)
	operator+(f32x4 other) const {
		f32x4 result;
		result.V = _mm_add_ps(V, other.V);
		return result;
	}
	force_inline(f32x4&)
	operator+=(f32x4 other) {
		*this = *this + other;
		return *this;
	}
	force_inline(f32x4)
	operator-(f32x4 other) const {
		f32x4 result;
		result.V = _mm_sub_ps(V, other.V);
		return result;
	}
	force_inline(f32x4&)
	operator-=(f32x4 other) {
		*this = *this - other;
		return *this;
	}
	force_inline(f32x4)
	operator*(f32x4 other) const {
		f32x4 result;
		result.V = _mm_mul_ps(V, other.V);
		return result;
	}
	force_inline(f32x4&)
	operator*=(f32x4 other) {
		*this = *this * other;
		return *this;
	}
	force_inline(f32x4)
	operator/(f32x4 other) const {
		f32x4 result;
		result.V = _mm_div_ps(V, other.V);
		return result;
	}
	force_inline(f32x4&)
	operator/=(f32x4 other) {
		*this = *this / other;
		return *this;
	}
	force_inline(u32x4)
	operator<(f32x4 other) const {
		u32x4 result;
		result.V = _mm_castps_si128(_mm_cmplt_ps(V, other.V));
		return result;
	}
	force_inline(u32x4)
	operator<=(f32x4 other) const {
		u32x4 result;
		result.V = _mm_castps_si128(_mm_cmple_ps(V, other.V));
		return result;
	}
	force_inline(u32x4)
	operator>(f32x4 other) const {
		u32x4 result;
		result.V = _mm_castps_si128(_mm_cmpgt_ps(V, other.V));
		return result;
	}
	force_inline(u32x4)
	operator>=(f32x4 other) const {
		u32x4 result;
		result.V = _mm_castps_si128(_mm_cmpge_ps(V, other.V));
		return result;
	}
	force_inline(u32x4)
	operator==(f32x4 other) const {
		u32x4 result;
		result.V = _mm_castps_si128(_mm_cmpeq_ps(V, other.V));
		return result;
	}
	force_inline(u32x4)
	operator!=(f32x4 other) const {
		u32x4 result;
		result.V = _mm_castps_si128(_mm_cmpneq_ps(V, other.V));
		return result;
	}

};

force_inline(f32x4)
sqrt(f32x4 a) {
	f32x4 result;
	result.V = _mm_sqrt_ps(a.V);
	return result;
}

force_inline(f32x4)
rsqrt(f32x4 a) {
	f32x4 result;
	result.V = _mm_rsqrt_ps(a.V);
	return result;
}

force_inline(f32x4)
reciprocal(f32x4 a) {
	f32x4 result;
	result.V = _mm_rcp_ps(a.V);
	return result;
}

force_inline(f32x4)
max(f32x4 a, f32x4 b) {
	f32x4 result;
	result.V = _mm_max_ps(a.V, b.V);
	return result;
}

force_inline(f32x4)
min(f32x4 a, f32x4 b) {
	f32x4 result;
	result.V = _mm_min_ps(a.V, b.V);
	return result;
}

#if __SSE3__
force_inline(f32)
horizontalAdd(f32x4 a) {
	// a.V = 0, 1, 2, 3
	__m128 shuf = _mm_movehdup_ps(a.V); // shuf = 1,   1,   3,   3
	__m128 sums = _mm_add_ps(a.V, shuf);// sums = 0+1, 1+1, 2+3, 3+3
	shuf = _mm_movehl_ps(shuf, sums);   // shuf = 2+3, 3+3, 3,   3
	sums = _mm_add_ss(sums, shuf);      // sums = 0+1+2+3, 1+1, 2+3, 3+3
	return _mm_cvtss_f32(sums);         // return 0+1+2+3
}
#else
force_inline(f32)
horizontalAdd(f32x4 a) {
	// a.V = 0, 1, 2, 3
	__m128 shuf = _mm_shuffle_ps(a.V, a.V, _MM_SHUFFLE(2, 3, 0, 1));
	// shuf = 1, 0, 3, 2
	__m128 sums = _mm_add_ps(shuf, a.V);
	// sums = 0+1, 0+1, 2+3, 2+3
	shuf = _mm_movehl_ps(shuf, sums);
	// shuf = 2+3, 2+3, 3, 2
	sums = _mm_add_ss(sums, shuf);
	// shuf = 0+1+2+3, 0+1, 2+3, 2+3
	return _mm_cvtss_f32(shuf); // return 0+1+2+3
}
#endif

#endif // AZCORE_SIMD_SSE2_HPP
