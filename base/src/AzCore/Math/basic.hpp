/*
	File: basic.hpp
	Author: Philip Haynes
*/

#ifndef AZCORE_MATH_BASIC_HPP
#define AZCORE_MATH_BASIC_HPP

#include "../basictypes.hpp"
// Use math.h because it puts the overloads in global namespace.
#include <math.h>

#include <emmintrin.h>

namespace AzCore {

const f64 halfpi64 = 1.5707963267948966;
const f64 pi64 = 3.1415926535897932;
const f64 tau64 = 6.2831853071795865;

const f32 halfpi = (f32)halfpi64;
const f32 pi = (f32)pi64;
const f32 tau = (f32)tau64;

enum Axis {
	X = 0,
	Y = 1,
	Z = 2
};

enum Plane {
	XY = 0,
	XZ = 1,
	XW = 2,
	YX = XY,
	YZ = 3,
	YW = 4,
	ZX = XZ,
	ZY = YZ,
	ZW = 5
};

// Returns the value of numerator/denominator rounded up instead of down
template<typename Int>
Int intDivCeil(Int numerator, Int denominator) {
	return (numerator + denominator - 1) / denominator;
}

// Takes a positive amplitude (the root-power quantity) factor and returns decibels
template<typename F>
F ampToDecibels(F amp) {
	AzAssert(amp >= F(0), "val must be positive");
	
	F result;
	if (amp == F(0)) {
		result = F(-INFINITY);
	} else {
		result = F(20) * log10(amp);
	}
	return result;
}

// Takes the value in decibels and returns the amplitude (the root-power quantity)
template<typename F>
F decibelsToAmp(F db) {
	F result;
	result = pow(F(10), db / F(20));
	return result;
}

} // namespace AzCore

template <typename T>
constexpr T square(T a) {
	return a * a;
}

template <typename T>
constexpr T min(T a, T b) {
	return (T)(a > b) * b + (T)(b >= a) * a;
}

template <typename T>
constexpr T max(T a, T b) {
	return (T)(a > b) * a + (T)(b >= a) * b;
}

inline f32 min(f32 a, f32 b) {
	_mm_store_ss(&a, _mm_min_ss(_mm_set_ss(a), _mm_set_ss(b)));
	return a;
}

inline f32 max(f32 a, f32 b) {
	_mm_store_ss(&a, _mm_max_ss(_mm_set_ss(a), _mm_set_ss(b)));
	return a;
}

inline f64 min(f64 a, f64 b) {
	_mm_store_sd(&a, _mm_min_sd(_mm_set_sd(a), _mm_set_sd(b)));
	return a;
}

inline f64 max(f64 a, f64 b) {
	_mm_store_sd(&a, _mm_max_sd(_mm_set_sd(a), _mm_set_sd(b)));
	return a;
}

template <typename T>
constexpr T median(T a, T b, T c) {
	return max(min(a, b), min(max(a, b), c));
}

template <typename T, typename... Args>
constexpr T max(T a, T b, Args... c) {
	return max(max(a, b), c...);
}

template <typename T>
constexpr T clamp(T a, T min, T max) {
	AzAssert(min <= max, "in clamp(): min > max. Maybe you meant to use median()?");
	return max * T(a > max) + min * T(a < min) + a * T(a <= max && a >= min);
}

inline f32 clamp(f32 a, f32 min, f32 max) {
	AzAssert(min <= max, "in clamp(): min > max. Maybe you meant to use median()?");
	return ::min(::max(a, min), max);
}

inline f64 clamp(f64 a, f64 min, f64 max) {
	AzAssert(min <= max, "in clamp(): min > max. Maybe you meant to use median()?");
	return ::min(::max(a, min), max);
}

template <typename T>
constexpr T clamp01(T a) {
	return a * T(a > T(0) && a < T(1)) + T(a >= T(1));
}

template <typename T>
constexpr T sign(T a) {
	return T(a >= T(0)) - T(a < T(0));
}

template <typename T>
constexpr T abs(T a) {
	return a * sign(a);
}

constexpr f32 norm(f32 a) {
	return abs<f32>(a);
}

constexpr f64 norm(f64 a) {
	return abs<f64>(a);
}

template <typename T, typename F>
constexpr T lerp(T a, T b, F factor) {
	return a + (b - a) * clamp01(factor);
}

template <typename T, typename F>
constexpr T cosInterp(T a, T b, F factor) {
	factor = F(0.5) - cos(F(AzCore::pi64) * clamp01(factor)) * F(0.5);
	return a + (b - a) * factor;
}

template <u32 order, typename T, typename F>
constexpr T ease(T a, T b, F factor) {
	factor = clamp01(factor);
	F factorP = F(1);
	F factorD = F(1);
	for (u32 i = 0; i < order; i++) {
		factorP *= factor;
		factorD *= F(1) - factor;
	}
	return a + (b - a) * factorP / (factorP + factorD);
}

template <typename T, typename F>
constexpr T decay(T a, T b, F halfLife, F timestep) {
	F fac = clamp01(pow(2, -timestep / halfLife));
	return b * (F(1.0) - fac) + a * fac;
}

template <typename T>
constexpr T map(T in, T minFrom, T maxFrom, T minTo, T maxTo) {
	return (in - minFrom) * (maxTo - minTo) / (maxFrom - minFrom) + minTo;
}

template <typename T>
constexpr T cubert(T a) {
	return sign(a) * pow(abs(a), T(1.0 / 3.0));
}

template <typename T>
constexpr T wrap(T a, T max) {
	AzAssert(max > 0, "wrap() with max <= 0 would hang or overflow >:P");
	while (a > max) {
		a -= max;
	}
	while (a < 0) {
		a += max;
	}
	return a;
}

#endif // AZCORE_MATH_BASIC_HPP
