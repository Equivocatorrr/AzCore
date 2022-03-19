/*
	File: Simd.hpp
	Author: Philip Haynes
	Implements AVX-supported SIMD.
*/
#ifndef AZCORE_SIMD_AVX_HPP
#define AZCORE_SIMD_AVX_HPP

#ifndef AZCORE_SIMD_SSE2_HPP
#include "SimdSSE2.hpp"
#endif

#include <immintrin.h> // AVX

#include "../basictypes.hpp"

#if __AVX2__
struct u32x8;
struct i32x8;
#endif // __AVX2__

struct f32x8;
struct f64x4;

#if defined(__GNUG__) && !defined(__clang__)
// Workaround this intrinsic not being defined for some reason
#define _mm256_cvtsi256_si32(V) _mm_cvtsi128_si32(_mm256_castsi256_si128(V))
#endif

#if __AVX2__
inline u32x8 max(u32x8 a, u32x8 b);
inline u32x8 min(u32x8 a, u32x8 b);
inline i32x8 max(i32x8 a, i32x8 b);
inline i32x8 min(i32x8 a, i32x8 b);

struct u32x8 {
	__m256i V;

	u32x8() = default;
	force_inline()
	u32x8(u32 other) { *this = other; }
	force_inline()
	u32x8(u32 other[8]) { *this = other; }
	force_inline()
	u32x8(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g, u32 h) {
		V = _mm256_setr_epi32(a, b, c, d, e, f, g, h);
	}
	force_inline()
	u32x8(i32x8 other);
	force_inline(u32x8&)
	operator=(u32 other) {
		V = _mm256_set1_epi32(other);
		return *this;
	}
	force_inline(u32x8&)
	operator=(u32 in[8]) {
		V = _mm256_setr_epi32(in[0], in[1], in[2], in[3], in[4], in[5], in[6], in[7]);
		return *this;
	}
	// Puts 8 values into dst
	force_inline(void)
	GetValues(u32 *dst) const {
		_mm256_storeu_si256((__m256i*)dst, V);
	}
	// Loads 8 values from src
	force_inline(void)
	SetValues(u32 *src) {
		V = _mm256_loadu_si256((const __m256i*)src);
	}

	template<u32 i>
	force_inline(u32)
	Get() const {
		u32 result;
		if constexpr (i == 0) {
			result = _mm256_cvtsi256_si32(V);
		} else {
			result = _mm256_extract_epi32(V, i);
		}
		return result;
	}

	force_inline(u32x8)
	operator+(u32x8 other) const {
		u32x8 result;
		result.V = _mm256_add_epi32(V, other.V);
		return result;
	}
	force_inline(u32x8)
	operator-(u32x8 other) const {
		u32x8 result;
		result.V = _mm256_sub_epi32(V, other.V);
		return result;
	}

	force_inline(u32x8)
	operator*(u32x8 other) const {
		u32x8 result;
		result.V = _mm256_mullo_epi32(V, other.V);
		return result;
	}
	force_inline(u32x8&)
	operator*=(u32x8 other) {
		*this = *this * other;
		return *this;
	}

	force_inline(u32x8)
	operator>=(u32x8 other) const {
		return max(*this, other) == *this;
	}
	force_inline(u32x8)
	operator<=(u32x8 other) const {
		return max(*this, other) == other;
	}
	force_inline(u32x8)
	operator>(u32x8 other) const {
		return ~operator<=(other);
	}
	force_inline(u32x8)
	operator<(u32x8 other) const {
		return ~operator>=(other);
	}
	force_inline(u32x8)
	operator==(u32x8 other) const {
		u32x8 result;
		result.V = _mm256_cmpeq_epi32(V, other.V);
		return result;
	}
	force_inline(u32x8)
	operator!=(u32x8 other) const {
		u32x8 result;
		result.V = _mm256_cmpeq_epi32(V, other.V);
		return ~result;
	}

	force_inline(u32x8)
	operator&(u32x8 other) const {
		u32x8 result;
		result.V = _mm256_and_si256(V, other.V);
		return result;
	}
	force_inline(u32x8&)
	operator&=(u32x8 other) {
		*this = *this & other;
		return *this;
	}
	force_inline(u32x8)
	operator|(u32x8 other) const {
		u32x8 result;
		result.V = _mm256_or_si256(V, other.V);
		return result;
	}
	force_inline(u32x8&)
	operator|=(u32x8 other) {
		*this = *this | other;
		return *this;
	}
	force_inline(u32x8)
	operator^(u32x8 other) const {
		u32x8 result;
		result.V = _mm256_xor_si256(V, other.V);
		return result;
	}
	force_inline(u32x8&)
	operator^=(u32x8 other) {
		*this = *this ^ other;
		return *this;
	}
	force_inline(u32x8)
	operator~() const {
		u32x8 result;
		result.V = _mm256_xor_si256(V, _mm256_set1_epi32(0xFFFFFFFF));
		return result;
	}
};

force_inline(u32x8)
max(u32x8 a, u32x8 b) {
	u32x8 result;
	result.V = _mm256_max_epu32(a.V, b.V);
	return result;
}

force_inline(u32x8)
min(u32x8 a, u32x8 b) {
	u32x8 result;
	result.V = _mm256_min_epu32(a.V, b.V);
	return result;
}

force_inline(u32x8)
AndNot(u32x8 lhs, u32x8 rhs) {
	u32x8 result;
	result.V = _mm256_andnot_si256(rhs.V, lhs.V);
	return result;
}

struct i32x8 {
	__m256i V;

	i32x8() = default;
	force_inline()
	i32x8(i32 other) { *this = other; }
	force_inline()
	i32x8(i32 other[8]) { *this = other; }
	force_inline()
	i32x8(i32 a, i32 b, i32 c, i32 d, i32 e, i32 f, i32 g, i32 h) {
		V = _mm256_setr_epi32(a, b, c, d, e, f, g, h);
	}
	inline i32x8(u32x8 other) : V(other.V) {}
	inline i32x8& operator=(i32 other) {
		V = _mm256_set1_epi32(other);
		return *this;
	}
	inline i32x8& operator=(i32 in[8]) {
		V = _mm256_setr_epi32(in[0], in[1], in[2], in[3], in[4], in[5], in[6], in[7]);
		return *this;
	}

	// Puts 8 values into dst
	force_inline(void)
	GetValues(i32 *dst) const {
		_mm256_storeu_si256((__m256i*)dst, V);
	}
	// Loads 8 values from src
	force_inline(void)
	SetValues(i32 *src) {
		V = _mm256_loadu_si256((const __m256i*)src);
	}

	template<u32 i>
	force_inline(i32)
	Get() const {
		i32 result;
		if constexpr (i == 0) {
			result = _mm256_cvtsi256_si32(V);
		} else {
			result = _mm256_extract_epi32(V, i);
		}
		return result;
	}

	force_inline(i32x8)
	operator*(u32x8 other) const {
		i32x8 result;
		result.V = _mm256_mullo_epi32(V, other.V);
		return result;
	}
	force_inline(i32x8&)
	operator*=(i32x8 other) {
		*this = *this * other;
		return *this;
	}

	force_inline(i32x8)
	operator+(i32x8 other) const {
		i32x8 result;
		result.V = _mm256_add_epi32(V, other.V);
		return result;
	}
	force_inline(i32x8&)
	operator+=(i32x8 other) {
		*this = *this + other;
		return *this;
	}
	force_inline(i32x8)
	operator-(i32x8 other) const {
		i32x8 result;
		result.V = _mm256_sub_epi32(V, other.V);
		return result;
	}
	force_inline(i32x8&)
	operator-=(i32x8 other) {
		*this = *this - other;
		return *this;
	}
	force_inline(u32x8)
	operator<(i32x8 other) const {
		return ~(operator==(other) | operator>(other));
	}
	force_inline(u32x8)
	operator>(i32x8 other) const {
		u32x8 result;
		result.V = _mm256_cmpgt_epi32(V, other.V);
		return result;
	}
	force_inline(u32x8)
	operator<=(i32x8 other) const {
		return ~operator>(other);
	}
	force_inline(u32x8)
	operator>=(i32x8 other) const {
		return operator==(other) | operator>(other);
	}
	force_inline(u32x8)
	operator==(i32x8 other) const {
		u32x8 result;
		result.V = _mm256_cmpeq_epi32(V, other.V);
		return result;
	}
};

force_inline()
u32x8::u32x8(i32x8 other) : V(other.V) {}

force_inline(i32x8)
max(i32x8 a, i32x8 b) {
	i32x8 result;
	result.V = _mm256_max_epi32(a.V, b.V);
	return result;
}

force_inline(i32x8)
min(i32x8 a, i32x8 b) {
	i32x8 result;
	result.V = _mm256_min_epi32(a.V, b.V);
	return result;
}

#endif // __AVX2__

struct f32x8 {
	__m256 V;

	f32x8() = default;
	force_inline()
	f32x8(f32 other) { *this = other; }
	force_inline()
	f32x8(i32 other) { *this = other; }
#if __AVX2__
	force_inline()
	f32x8(i32x8 other) { *this = other; }
	force_inline(f32x8&)
	operator=(i32x8 other) {
		V = _mm256_cvtepi32_ps(other.V);
		return *this;
	}
#endif // __AVX2__
	force_inline()
	f32x8(f32 other[8]) { *this = other; }
	force_inline()
	f32x8(f32 a, f32 b, f32 c, f32 d, f32 e, f32 f, f32 g, f32 h) {
		V = _mm256_setr_ps(a, b, c, d, e, f, g, h);
	}
	force_inline(f32x8&)
	operator=(f32 other) {
		V = _mm256_set1_ps(other);
		return *this;
	}
	force_inline(f32x8&)
	operator=(i32 other) {
		V = _mm256_set1_ps((f32)other);
		return *this;
	}
	force_inline(f32x8&)
	operator=(f32 in[8]) {
		V = _mm256_setr_ps(in[0], in[1], in[2], in[3], in[4], in[5], in[6], in[7]);
		return *this;
	}

	// Puts 8 values into dst
	force_inline(void)
	GetValues(f32 *dst) const {
		_mm256_storeu_ps(dst, V);
	}
	// Loads 8 values from src
	force_inline(void)
	SetValues(f32 *src) {
		V = _mm256_loadu_ps(src);
	}

	template<u32 i>
	force_inline(f32)
	Get() const {
		f32 result;
		if constexpr (i == 0) {
			result = _mm256_cvtss_f32(V);
		} else {
			result = _mm256_cvtss_f32(_mm256_shuffle_ps(V, V, _MM_SHUFFLE(i, i, i, i)));
		}
		return result;
	}

	force_inline(f32x8)
	operator+(f32x8 other) const {
		f32x8 result;
		result.V = _mm256_add_ps(V, other.V);
		return result;
	}
	force_inline(f32x8&)
	operator+=(f32x8 other) {
		*this = *this + other;
		return *this;
	}
	force_inline(f32x8)
	operator-(f32x8 other) const {
		f32x8 result;
		result.V = _mm256_sub_ps(V, other.V);
		return result;
	}
	force_inline(f32x8&)
	operator-=(f32x8 other) {
		*this = *this - other;
		return *this;
	}
	force_inline(f32x8)
	operator*(f32x8 other) const {
		f32x8 result;
		result.V = _mm256_mul_ps(V, other.V);
		return result;
	}
	force_inline(f32x8&)
	operator*=(f32x8 other) {
		*this = *this * other;
		return *this;
	}
	force_inline(f32x8)
	operator/(f32x8 other) const {
		f32x8 result;
		result.V = _mm256_div_ps(V, other.V);
		return result;
	}
	force_inline(f32x8&)
	operator/=(f32x8 other) {
		*this = *this / other;
		return *this;
	}
#if __AVX2__
	force_inline(u32x8)
	operator<(f32x8 other) const {
		u32x8 result;
		result.V = _mm256_castps_si256(_mm256_cmp_ps(V, other.V, _CMP_LT_OQ));
		return result;
	}
	force_inline(u32x8)
	operator<=(f32x8 other) const {
		u32x8 result;
		result.V = _mm256_castps_si256(_mm256_cmp_ps(V, other.V, _CMP_LE_OQ));
		return result;
	}
	force_inline(u32x8)
	operator>(f32x8 other) const {
		u32x8 result;
		result.V = _mm256_castps_si256(_mm256_cmp_ps(V, other.V, _CMP_GT_OQ));
		return result;
	}
	force_inline(u32x8)
	operator>=(f32x8 other) const {
		u32x8 result;
		result.V = _mm256_castps_si256(_mm256_cmp_ps(V, other.V, _CMP_GE_OQ));
		return result;
	}
	force_inline(u32x8)
	operator==(f32x8 other) const {
		u32x8 result;
		result.V = _mm256_castps_si256(_mm256_cmp_ps(V, other.V, _CMP_EQ_OQ));
		return result;
	}
	force_inline(u32x8)
	operator!=(f32x8 other) const {
		u32x8 result;
		result.V = _mm256_castps_si256(_mm256_cmp_ps(V, other.V, _CMP_NEQ_OQ));
		return result;
	}
#endif // __AVX2__
};

force_inline(f32x8)
sqrt(f32x8 a) {
	f32x8 result;
	result.V = _mm256_sqrt_ps(a.V);
	return result;
}

force_inline(f32x8)
rsqrt(f32x8 a) {
	f32x8 result;
	result.V = _mm256_rsqrt_ps(a.V);
	return result;
}

force_inline(f32x8)
reciprocal(f32x8 a) {
	f32x8 result;
	result.V = _mm256_rcp_ps(a.V);
	return result;
}

force_inline(f32x8)
max(f32x8 a, f32x8 b) {
	f32x8 result;
	result.V = _mm256_max_ps(a.V, b.V);
	return result;
}

force_inline(f32x8)
min(f32x8 a, f32x8 b) {
	f32x8 result;
	result.V = _mm256_min_ps(a.V, b.V);
	return result;
}

struct f64x4 {
	__m256d V;

	f64x4() = default;
	force_inline()
	f64x4(f64 other) { *this = other; }
	force_inline()
	f64x4(i32 other) { *this = other; }
	force_inline()
	f64x4(i32x4 other) { *this = other; }
	force_inline()
	f64x4(f64 other[4]) { *this = other; }
	force_inline()
	f64x4(f64 a, f64 b, f64 c, f64 d) {
		V = _mm256_setr_pd(a, b, c, d);
	}
	force_inline(f64x4&)
	operator=(f64 other) {
		V = _mm256_set1_pd(other);
		return *this;
	}
	force_inline(f64x4&)
	operator=(i32 other) {
		V = _mm256_set1_pd((f64)other);
		return *this;
	}
	force_inline(f64x4&)
	operator=(i32x4 other) {
		V = _mm256_cvtepi32_pd(other.V);
		return *this;
	}
	force_inline(f64x4&)
	operator=(f64 in[4]) {
		V = _mm256_setr_pd(in[0], in[1], in[2], in[3]);
		return *this;
	}

	// Puts 4 values into dst
	force_inline(void)
	GetValues(f64 *dst) const {
		_mm256_storeu_pd(dst, V);
	}
	// Loads 4 values from src
	force_inline(void)
	SetValues(f64 *src) {
		V = _mm256_loadu_pd(src);
	}

	template<u32 i>
	force_inline(f64)
	Get() const {
		f64 result;
		if constexpr (i == 0) {
			result = _mm256_cvtsd_f64(V);
		} else {
			result = _mm256_cvtsd_f64(_mm256_shuffle_pd(V, V, _MM_SHUFFLE(i, i, i, i)));
		}
		return result;
	}

	force_inline(f64x4)
	operator-() const {
		f64x4 result;
		result.V = _mm256_sub_pd(_mm256_set1_pd(0.0), V);
		return result;
	}
	force_inline(f64x4)
	operator+(f64x4 other) const {
		f64x4 result;
		result.V = _mm256_add_pd(V, other.V);
		return result;
	}
	force_inline(f64x4&)
	operator+=(f64x4 other) {
		*this = *this + other;
		return *this;
	}
	force_inline(f64x4)
	operator-(f64x4 other) const {
		f64x4 result;
		result.V = _mm256_sub_pd(V, other.V);
		return result;
	}
	force_inline(f64x4&)
	operator-=(f64x4 other) {
		*this = *this - other;
		return *this;
	}
	force_inline(f64x4)
	operator*(f64x4 other) const {
		f64x4 result;
		result.V = _mm256_mul_pd(V, other.V);
		return result;
	}
	force_inline(f64x4&)
	operator*=(f64x4 other) {
		*this = *this * other;
		return *this;
	}
	force_inline(f64x4)
	operator/(f64x4 other) const {
		f64x4 result;
		result.V = _mm256_div_pd(V, other.V);
		return result;
	}
	force_inline(f64x4&)
	operator/=(f64x4 other) {
		*this = *this / other;
		return *this;
	}
	force_inline(u32x4)
	operator<(f64x4 other) const {
		u32x4 result;
		result.V = _mm_castps_si128(_mm256_cvtpd_ps(_mm256_cmp_pd(V, other.V, _CMP_LT_OQ)));
		return result;
	}
	force_inline(u32x4)
	operator<=(f64x4 other) const {
		u32x4 result;
		result.V = _mm_castps_si128(_mm256_cvtpd_ps(_mm256_cmp_pd(V, other.V, _CMP_LE_OQ)));
		return result;
	}
	force_inline(u32x4)
	operator>(f64x4 other) const {
		u32x4 result;
		result.V = _mm_castps_si128(_mm256_cvtpd_ps(_mm256_cmp_pd(V, other.V, _CMP_GT_OQ)));
		return result;
	}
	force_inline(u32x4)
	operator>=(f64x4 other) const {
		u32x4 result;
		result.V = _mm_castps_si128(_mm256_cvtpd_ps(_mm256_cmp_pd(V, other.V, _CMP_GE_OQ)));
		return result;
	}
	force_inline(u32x4)
	operator==(f64x4 other) const {
		u32x4 result;
		result.V = _mm_castps_si128(_mm256_cvtpd_ps(_mm256_cmp_pd(V, other.V, _CMP_EQ_OQ)));
		return result;
	}
	force_inline(u32x4)
	operator!=(f64x4 other) const {
		u32x4 result;
		result.V = _mm_castps_si128(_mm256_cvtpd_ps(_mm256_cmp_pd(V, other.V, _CMP_NEQ_OQ)));
		return result;
	}

};

force_inline(f64x4)
sqrt(f64x4 a) {
	f64x4 result;
	result.V = _mm256_sqrt_pd(a.V);
	return result;
}

force_inline(f64x4)
max(f64x4 a, f64x4 b) {
	f64x4 result;
	result.V = _mm256_max_pd(a.V, b.V);
	return result;
}

force_inline(f64x4)
min(f64x4 a, f64x4 b) {
	f64x4 result;
	result.V = _mm256_min_pd(a.V, b.V);
	return result;
}

#endif // AZCORE_SIMD_AVX_HPP
