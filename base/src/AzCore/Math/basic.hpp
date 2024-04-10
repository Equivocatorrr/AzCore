/*
	File: basic.hpp
	Author: Philip Haynes
*/

#ifndef AZCORE_MATH_BASIC_HPP
#define AZCORE_MATH_BASIC_HPP

#include "../basictypes.hpp"
#include "../Assert.hpp"
// Use math.h because it puts the overloads in global namespace.
#include <math.h>

#include <emmintrin.h>

#include <type_traits>

namespace AzCore {

constexpr f64 halfpi64 = 1.5707963267948966;
constexpr f64 pi64 = 3.1415926535897932;
constexpr f64 tau64 = 6.2831853071795865;
constexpr f64 invPi64 = 1.0 / pi64;
constexpr f64 invTau64 = 1.0 / tau64;

constexpr f32 halfpi = (f32)halfpi64;
constexpr f32 pi = (f32)pi64;
constexpr f32 tau = (f32)tau64;
constexpr f32 invPi = (f32)invPi64;
constexpr f32 invTau = (f32)invTau64;

template<typename T> struct Pi;
template<> struct Pi<f32> {
	static constexpr f32 value = pi;
};
template<> struct Pi<f64> {
	static constexpr f64 value = pi64;
};
template<typename T> struct Tau;
template<> struct Tau<f32> {
	static constexpr f32 value = tau;
};
template<> struct Tau<f64> {
	static constexpr f64 value = tau64;
};

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

u64 GreatestCommonFactor(u64 a, u64 b);

u64 GreatestCommonFactor(std::initializer_list<u64> list);

inline u64 LeastCommonMultiple(u64 a, u64 b) {
	return a * b / GreatestCommonFactor(a, b);
}

} // namespace AzCore

template <typename T>
constexpr T square(T a) {
	return a * a;
}

template <typename T>
constexpr T min(T a, T b) {
	return a <= b ? a : b;
}

template <typename T>
constexpr T max(T a, T b) {
	return a >= b ? a : b;
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

template <typename T, typename... Args>
constexpr T min(T a, T b, Args... c) {
	return min(min(a, b), c...);
}

template <typename T>
constexpr T clamp(T a, T min, T max) {
	AzAssert(min <= max, "in clamp(): min > max. Maybe you meant to use median()?");
	return ::min(::max(a, min), max);
}

template <typename T>
constexpr T clamp01(T a) {
	return clamp(a, T(0), T(1));
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
constexpr T lerpUnclamped(T a, T b, F factor) {
	return a * (F(1) - factor) + b * factor;
}

template <typename T, typename F>
constexpr T lerp(T a, T b, F factor) {
	factor = clamp01(factor);
	return lerpUnclamped(a, b, factor);
}

// Uses the cosine function to make an S-curve
template <typename T, typename F>
constexpr T cosInterp(T a, T b, F factor) {
	factor = clamp01(factor);
	factor = F(0.5) - cos(az::Pi<F>::value * factor) * F(0.5);
	return lerpUnclamped(a, b, factor);
}

// This formula chooses tangents that average the lines at each vertex
// NOTE: We could probably improve this to pick a tangent that keeps the output range between 0 and 1.
// Currently the output can go from -0.125 to 1.125
// Alternatively we could conditionally move the inputs to limit the range
template <typename T, typename F>
constexpr T cubicInterp(T a_0, T a, T b, T b_1, F factor) {
	factor = clamp01(factor);
	F f2 = square(factor);
	F f3 = f2 * factor;
	return a_0*(F(-0.5)*f3 + f2 + F(-0.5)*factor)
	     + a*(F(1.5)*f3 - F(2.5)*f2 + F(1))
	     + b*(F(-1.5)*f3 + F(2)*f2 + F(0.5)*factor)
	     + b_1*(F(0.5)*(f3 - f2));
}

// Behaves similarly to smoothInterp, but with the given tangents (1st derivatives) at the endpoints
template <typename T, typename F>
constexpr T hermiteInterp(T a, T a_tangent, T b, T b_tangent, F factor) {
	factor = clamp01(factor);
	F f2 = square(factor);
	F f3 = f2 * factor;
	F endpointBasis = -F(2)*f3 + F(3)*f2;
	return lerpUnclamped(a, b, endpointBasis)
	     + (f3 - F(2)*f2 + factor) * a_tangent + (f3 - f2) * b_tangent;
}

template <typename F>
constexpr F smoothFactor(F x) {
	return x * x * (F(3) - F(2) * x);
}

template <typename F>
constexpr F smootherFactor(F x) {
	return x * x * x * (F(10) + x * (F(-15) + F(6) * x));
}

// 1st derivative at endpoints is zero
template <typename T, typename F>
constexpr T smoothInterp(T a, T b, F factor) {
	factor = clamp01(factor);
	factor = smoothFactor(factor);
	return lerpUnclamped(a, b, factor);
}

// 1st and 2nd derivatives at endpoints are zero
template <typename T, typename F>
constexpr T smootherInterp(T a, T b, F factor) {
	factor = clamp01(factor);
	factor = smootherFactor(factor);
	return lerpUnclamped(a, b, factor);
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
	factor = factorP / (factorP + factorD);
	return lerpUnclamped(a, b, factor);
}

template <typename F>
constexpr F decayFactor(F halfLife, F timestep) {
	return 1 - clamp01(pow(2, -timestep / halfLife));
}

template <typename T, typename F>
constexpr T decay(T a, T b, F halfLife, F timestep) {
	F factor = decayFactor(halfLife, timestep);
	return lerpUnclamped(a, b, factor);
}

template <typename T>
constexpr T map(T in, T minFrom, T maxFrom, T minTo, T maxTo) {
	return (in - minFrom) * (maxTo - minTo) / (maxFrom - minFrom) + minTo;
}

template <typename T>
constexpr T cubert(T a) {
	return sign(a) * pow(abs(a), T(1.0 / 3.0));
}

// Always returns a value with the same signed-ness as b. This is the actual modulo operator, not just remainder like % is.
template<typename T>
constexpr T mod(T a, T b) {
	T remainder = a % b;
	if ((a > 0 && remainder < 0) || (a > 0 && remainder > 0)) {
		remainder += a;
	}
	return remainder;
}

template <typename T>
constexpr T wrap(T a, T max) {
	if constexpr (std::is_floating_point_v<T>) {
		return fmod(a, max) + max * T(a < T(0));
	} else {
		return mod(a, max);
	}
}


#endif // AZCORE_MATH_BASIC_HPP
