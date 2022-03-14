/*
	File: Simd.hpp
	Author: Philip Haynes
	Implements MMX SIMD. This probably shouldn't really be used.
*/
#ifndef AZCORE_SIMD_MMX_HPP
#define AZCORE_SIMD_MMX_HPP

#include <string.h>

#include <mmintrin.h>  // MMX

#if __SSE2__
#warning "Using MMX when SSE2 is available is probably not advisable."
#endif

#include "../basictypes.hpp"

struct u32x2;
struct i32x2;

// TODO: Implement these
struct u16x4;
struct i16x4;

struct u32x2 {
	__m64 V;

	u32x2() = default;
	force_inline()
	u32x2(u32 other) { *this = other; }
	force_inline()
	u32x2(u32 other[2]) { *this = other; }
	force_inline()
	u32x2(u32 a, u32 b) {
		V = _mm_setr_pi32(a, b);
	}
	force_inline()
	u32x2(i32x2 other);
	force_inline(u32x2&)
	operator=(u32 other) {
		V = _mm_set1_pi32(other);
		return *this;
	}
	force_inline(u32x2&)
	operator=(u32 in[2]) {
		V = _mm_setr_pi32(in[0], in[1]);
		return *this;
	}
	// Puts 2 values into dst
	force_inline(void)
	GetValues(u32 *dst) const {
		dst[0] = Get<0>();
		dst[1] = Get<1>();
	}
	// Loads 2 values from src
	force_inline(void)
	SetValues(u32 *src) {
		*this = src;
	}

	template<u32 i>
	force_inline(u32)
	Get() const {
		u32 result;
		if constexpr (i == 0) {
			result = _mm_cvtsi64_si32(V);
		} else {
			result = _mm_cvtsi64_si32(_mm_shuffle_pi16(V, _MM_SHUFFLE(1, 0, 3, 2)));
		}
		return result;
	}

	force_inline(u32x2)
	operator+(u32x2 other) const {
		u32x2 result;
		result.V = _mm_add_pi32(V, other.V);
		return result;
	}
	force_inline(u32x2)
	operator-(u32x2 other) const {
		u32x2 result;
		result.V = _mm_sub_pi32(V, other.V);
		return result;
	}

	force_inline(u32x2)
	operator==(u32x2 other) const {
		u32x2 result;
		result.V = _mm_cmpeq_pi32(V, other.V);
		return result;
	}
	force_inline(u32x2)
	operator!=(u32x2 other) const {
		return ~(*this == other);
	}
	force_inline(u32x2)
	operator&(u32x2 other) const {
		u32x2 result;
		result.V = _mm_and_si64(V, other.V);
		return result;
	}
	force_inline(u32x2&)
	operator&=(u32x2 other) {
		*this = *this & other;
		return *this;
	}
	force_inline(u32x2)
	operator|(u32x2 other) const {
		u32x2 result;
		result.V = _mm_or_si64(V, other.V);
		return result;
	}
	force_inline(u32x2&)
	operator|=(u32x2 other) {
		*this = *this | other;
		return *this;
	}
	force_inline(u32x2)
	operator^(u32x2 other) const {
		u32x2 result;
		result.V = _mm_xor_si64(V, other.V);
		return result;
	}
	force_inline(u32x2&)
	operator^=(u32x2 other) {
		*this = *this ^ other;
		return *this;
	}
	force_inline(u32x2)
	operator~() const {
		u32x2 result;
		result.V = _mm_xor_si64(V, _mm_set1_pi32(0xFFFFFFFF));
		return result;
	}
};

force_inline(u32x2)
AndNot(u32x2 lhs, u32x2 rhs) {
	u32x2 result;
	result.V = _mm_andnot_si64(rhs.V, lhs.V);
	return result;
}

force_inline(u32)
horizontalAdd(u32x2 a) {
	// a.V = 0, 1
	__m64 shuf = _mm_shuffle_pi16(a.V, _MM_SHUFFLE(1, 0, 3, 2));
	// shuf = 1, 0
	__m64 sums = _mm_add_pi32(shuf, a.V);
	// sums = 0+1, 1+0
	return _mm_cvtsi64_si32(sums); // return 0+1
}



struct i32x2 {
	__m64 V;

	i32x2() = default;
	force_inline()
	i32x2(i32 other) { *this = other; }
	force_inline()
	i32x2(i32 other[2]) { *this = other; }
	force_inline()
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
	force_inline(void)
	GetValues(i32 *dst) const {
		dst[0] = Get<0>();
		dst[1] = Get<1>();
	}
	// Loads 2 values from src
	force_inline(void)
	SetValues(i32 *src) {
		*this = src;
	}

	template<u32 i>
	force_inline(i32)
	Get() const {
		i32 result;
		if constexpr (i == 0) {
			result = _mm_cvtsi64_si32(V);
		} else {
			result = _mm_cvtsi64_si32(_mm_shuffle_pi16(V, _MM_SHUFFLE(1, 0, 3, 2)));
		}
		return result;
	}

	force_inline(i32x2)
	operator+(i32x2 other) const {
		i32x2 result;
		result.V = _mm_add_pi32(V, other.V);
		return result;
	}
	force_inline(i32x2&)
	operator+=(i32x2 other) {
		*this = *this + other;
		return *this;
	}
	force_inline(i32x2)
	operator-(i32x2 other) const {
		i32x2 result;
		result.V = _mm_sub_pi32(V, other.V);
		return result;
	}
	force_inline(i32x2&)
	operator-=(i32x2 other) {
		*this = *this - other;
		return *this;
	}
	force_inline(u32x2)
	operator<(i32x2 other) const {
		return ~operator>=(other);
	}
	force_inline(u32x2)
	operator>(i32x2 other) const {
		u32x2 result;
		result.V = _mm_cmpgt_pi32(V, other.V);
		return result;
	}
	force_inline(u32x2)
	operator<=(i32x2 other) const {
		return ~operator>(other);
	}
	force_inline(u32x2)
	operator>=(i32x2 other) const {
		return operator>(other) | operator==(other);
	}
	force_inline(u32x2)
	operator==(i32x2 other) const {
		u32x2 result;
		result.V = _mm_cmpeq_pi32(V, other.V);
		return result;
	}
};

force_inline()
u32x2::u32x2(i32x2 other) : V(other.V) {}

force_inline(i32)
horizontalAdd(i32x2 a) {
	return (i32)horizontalAdd(u32x2(a));
}

#endif // AZCORE_SIMD_SSE2_HPP
