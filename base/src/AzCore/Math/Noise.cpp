/*
	File: Noise.cpp
	Author: Philip Haynes
*/

#include "Noise.hpp"

namespace AzCore {

namespace Noise {

constexpr u64 PRIME1 = 123456789133;
constexpr u64 PRIME2 = 456789123499;
constexpr u64 PRIME3 = 789123456817;
constexpr u64 PRIME4 = 147258369157;
constexpr u64 PRIME5 = 258369147317;

u64 hash(u64 x) {
	u64 result = x + PRIME2;
	result *= PRIME1;
	result ^= result >> 31;
	result ^= result << 21;
	result ^= result >> 13;
	return result;
}

u64 hash(u64 x, u64 y) {
	u64 result = x + PRIME4;
	result *= PRIME1;
	result ^= result >> 31;
	result ^= result << 29;
	result += y;
	result *= PRIME2;
	result ^= result >> 13;
	result ^= result << 11;
	result *= PRIME3;
	result ^= result >> 17;
	result ^= result << 22;
	return result;
}

u64 hash(u64 x, u64 y, u64 z) {
	u64 result = x + PRIME4;
	result *= PRIME1;
	result ^= result >> 31;
	result ^= result << 29;
	result += y;
	result *= PRIME2;
	result ^= result >> 13;
	result ^= result << 11;
	result += z;
	result *= PRIME3;
	result ^= result >> 17;
	result ^= result << 22;
	return result;
}

u64 hash(u64 x, u64 y, u64 z, u64 w) {
	u64 result = x + PRIME5;
	result *= PRIME1;
	result ^= result >> 31;
	result ^= result << 29;
	result += y;
	result *= PRIME2;
	result ^= result >> 13;
	result ^= result << 11;
	result += z;
	result *= PRIME3;
	result ^= result >> 13;
	result ^= result << 22;
	result += w;
	result *= PRIME4;
	result ^= result >> 19;
	result ^= result << 17;
	return result;
}

template<>
f32 hashedToFloat<f32>(u64 x) {
	f32 result;
	// Largest value that can sit in the mantissa of an f32
	x &= 0x7fffff;
	result = (f32)x / (f32)0x7fffff;
	return result;
}

template<>
f64 hashedToFloat<f64>(u64 x) {
	f64 result;
	// Largest value that can sit in the mantissa of an f64
	x &= 0xfffffffffffff;
	result = (f64)x / (f64)0xfffffffffffff;
	return result;
}

template <typename F>
F linearNoise(f64 x, u64 seed) {
	f64 wholef = floor(x);
	u64 whole = wholef;
	x -= wholef;
	F p1 = whiteNoise<F>(whole, seed);
	F p2 = whiteNoise<F>(whole+1, seed);
	return lerp(p1, p2, x);
}

template
f32 linearNoise<f32>(f64 x, u64 seed);
template
f64 linearNoise<f64>(f64 x, u64 seed);

template <typename F>
F perlinNoise(f64 x, u64 seed) {
	f64 wholef = floor(x);
	u64 whole = wholef;
	x -= wholef;
	// Tangent at endpoints, range between -2 and 2 gets us an output between 0 and 1
	F t1 = whiteNoise<F>(whole, seed) * F(4) - F(2);
	F t2 = whiteNoise<F>(whole+1, seed) * F(4) - F(2);
	// TODO: using the full hermite function is wasteful, so extract just the tangent parts
	return hermiteInterp(F(0.5), t1, F(0.5), t2, x);
}

template
f32 perlinNoise<f32>(f64 x, u64 seed);
template
f64 perlinNoise<f64>(f64 x, u64 seed);

template <typename F>
F perlinNoise(vec2d pos, u64 seed) {
	vec2d wholef = vec2d(floor(pos.x), floor(pos.y));
	vec2i whole = wholef;
	pos -= wholef;
	vec2_t<F> posf = pos;
	// unit vectors at corners
	vec2_t<F> a1 = vec2_t<F>::UnitVecFromAngle(Tau<F>::value * whiteNoise<F>(whole, seed));
	vec2_t<F> a2 = vec2_t<F>::UnitVecFromAngle(Tau<F>::value * whiteNoise<F>(vec2i(whole.x+1, whole.y), seed));
	vec2_t<F> a3 = vec2_t<F>::UnitVecFromAngle(Tau<F>::value * whiteNoise<F>(vec2i(whole.x, whole.y+1), seed));
	vec2_t<F> a4 = vec2_t<F>::UnitVecFromAngle(Tau<F>::value * whiteNoise<F>(vec2i(whole.x+1, whole.y+1), seed));
	F n1 = dot(a1, vec2_t<F>(posf.x, posf.y));
	F n2 = dot(a2, vec2_t<F>(posf.x-F(1), posf.y));
	F n3 = dot(a3, vec2_t<F>(posf.x, posf.y-F(1)));
	F n4 = dot(a4, vec2_t<F>(posf.x-F(1), posf.y-F(1)));
	return smootherInterp(
		smootherInterp(n1, n2, posf.x),
		smootherInterp(n3, n4, posf.x),
		posf.y
	) * F(0.70) + F(0.5);
}

template
f32 perlinNoise<f32>(vec2d pos, u64 seed);
template
f64 perlinNoise<f64>(vec2d pos, u64 seed);

template <typename F>
F linearNoise(vec2d pos, u64 seed) {
	vec2d wholef = vec2d(floor(pos.x), floor(pos.y));
	vec2i whole = wholef;
	pos -= wholef;
	F p1 = whiteNoise<F>(whole, seed);
	F p2 = whiteNoise<F>(vec2i(whole.x+1, whole.y), seed);
	F p3 = whiteNoise<F>(vec2i(whole.x, whole.y+1), seed);
	F p4 = whiteNoise<F>(vec2i(whole.x+1, whole.y+1), seed);
	return lerp(
		lerp(p1, p2, pos.x),
		lerp(p3, p4, pos.x),
		pos.y
	);
}

template
f32 linearNoise<f32>(vec2d pos, u64 seed);
template
f64 linearNoise<f64>(vec2d pos, u64 seed);

template <typename F>
F simplexNoise(vec2d pos, u64 seed) {
	// constexpr F skewFactor = (sqrt(3.0)-1.0)/2.0;
	constexpr F skewFactor = F(0.36602540378443865);
	// constexpr F skewFactorInv = (1.0 - 1.0/sqrt(3.0))/2.0;
	constexpr F skewFactorInv = F(0.21132486540518711);
	pos += vec2d((pos.x + pos.y) * skewFactor);
	vec2d wholed = vec2d(floor(pos.x), floor(pos.y));
	vec2i whole = wholed;
	pos -= wholed;
	pos -= vec2d((pos.x + pos.y) * skewFactorInv);
	vec2i middleOffset = vec2i((int)(pos.x > pos.y), (int)(pos.y >= pos.x));
	vec2_t<F> g_a = vec2_t<F>::UnitVecFromAngle(Tau<F>::value * whiteNoise<F>(whole, seed));
	vec2_t<F> g_b = vec2_t<F>::UnitVecFromAngle(Tau<F>::value * whiteNoise<F>(whole + middleOffset, seed));
	vec2_t<F> g_c = vec2_t<F>::UnitVecFromAngle(Tau<F>::value * whiteNoise<F>(whole + vec2i(1), seed));
	vec2_t<F> posf = pos;
	vec2_t<F> wholef = wholed;
	wholef -= vec2_t<F>((wholef.x + wholef.y) * skewFactorInv);
	vec2_t<F> middleOffsetf = middleOffset;
	vec2_t<F> endOffsetf = vec2_t<F>(F(1)-F(2)*skewFactorInv);
	middleOffsetf -= vec2_t<F>(skewFactorInv);
	// a, b, c are the points of the actual equilateral triangle
	vec2_t<F> a = wholef;
	vec2_t<F> b = wholef + middleOffsetf;
	vec2_t<F> c = wholef + endOffsetf;
	// vectors from points to pos
	vec2_t<F> ap = posf;
	vec2_t<F> bp = posf - middleOffsetf;
	vec2_t<F> cp = posf - endOffsetf;
	// Wavelets for the points
	F w_a = dot(g_a, ap);
	F w_b = dot(g_b, bp);
	F w_c = dot(g_c, cp);
	// Point wavelet contribution factors
	F f_a = max(F(0.5) - normSqr(ap), F(0));
	F f_b = max(F(0.5) - normSqr(bp), F(0));
	F f_c = max(F(0.5) - normSqr(cp), F(0));
	// 4th power
	f_a *= f_a; f_a *= f_a;
	f_b *= f_b; f_b *= f_b;
	f_c *= f_c; f_c *= f_c;
	return (w_a * f_a + w_b * f_b + w_c * f_c) * F(50) + F(0.5);
}

template
f32 simplexNoise<f32>(vec2d pos, u64 seed);
template
f64 simplexNoise<f64>(vec2d pos, u64 seed);

template <typename F>
F cosineNoise(vec2d pos, u64 seed) {
	vec2d wholef = vec2d(floor(pos.x), floor(pos.y));
	vec2i whole = wholef;
	pos -= wholef;
	F p1 = whiteNoise<F>(whole, seed);
	F p2 = whiteNoise<F>(vec2i(whole.x+2, whole.y), seed);
	F p3 = whiteNoise<F>(vec2i(whole.x, whole.y+2), seed);
	F p4 = whiteNoise<F>(vec2i(whole.x+2, whole.y+1), seed);
	return cosInterp(
		cosInterp(p1, p2, pos.x),
		cosInterp(p3, p4, pos.x),
		pos.y
	);
}

template
f32 cosineNoise<f32>(vec2d pos, u64 seed);
template
f64 cosineNoise<f64>(vec2d pos, u64 seed);

template <typename F>
F cubicNoise(vec2d pos, u64 seed) {
	vec2d wholef = vec2d(floor(pos.x), floor(pos.y));
	vec2i whole = wholef;
	pos -= wholef;
	F p[16];
	for (i32 y = 0; y < 4; y++) {
		for (i32 x = 0; x < 4; x++) {
			p[y*4 + x] = whiteNoise<F>(vec2i(whole.x + x-1, whole.y + y-1), seed);
		}
	}
	F result = cubicInterp(
		(cubicInterp(p[ 0], p[ 1], p[ 2], p[ 3], pos.x) + F(0.125)) * F(1.0/1.25),
		(cubicInterp(p[ 4], p[ 5], p[ 6], p[ 7], pos.x) + F(0.125)) * F(1.0/1.25),
		(cubicInterp(p[ 8], p[ 9], p[10], p[11], pos.x) + F(0.125)) * F(1.0/1.25),
		(cubicInterp(p[12], p[13], p[14], p[15], pos.x) + F(0.125)) * F(1.0/1.25),
		pos.y
	);
	// Since cubic interpolation can go 0.125f past the boundaries
	result = (result + F(0.125)) * F(1.0/1.25);
	return result;
}

template
f32 cubicNoise<f32>(vec2d pos, u64 seed);
template
f64 cubicNoise<f64>(vec2d pos, u64 seed);

} // namespace Noise

} // namespace AzCore