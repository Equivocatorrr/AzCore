/*
	File: Noise.hpp
	Author: Philip Haynes
	Purely functional noise algorithms that output floating point values between 0 and 1.
*/

#ifndef AZCORE_NOISE_HPP
#define AZCORE_NOISE_HPP

#include "../basictypes.hpp"
#include "../math.hpp"
#include "../Memory/ArrayWithBucket.hpp"

namespace AzCore {

namespace Noise {

u64 hash(u64 x);
u64 hash(u64 x, u64 y);
u64 hash(u64 x, u64 y, u64 z);
u64 hash(u64 x, u64 y, u64 z, u64 w);

// Converts the hash x to a float between 0 and 1
template <typename F>
F hashedToFloat(u64 x);

template<>
f32 hashedToFloat<f32>(u64 x);

template<>
f64 hashedToFloat<f64>(u64 x);

template <typename F>
inline F whiteNoise(u64 x) {
	return hashedToFloat<F>(hash(x));
}

template <typename F>
inline F whiteNoise(u64 x, u64 seed) {
	return hashedToFloat<F>(hash(x, seed));
}

// This way we don't break strict aliasing or alignment rules.
constexpr u64 BitCastVec2iToU64(vec2i vec) {
	// Cast to u32 first because otherwise it sign-extends to 64-bits
	u64 value = (u64)(u32)vec.x | ((u64)(u32)vec.y << 32);
	return value;
}

template <typename F>
inline F whiteNoise(vec2i pos, u64 seed) {
	return hashedToFloat<F>(hash(BitCastVec2iToU64(pos), seed));
}

template <typename F>
inline F whiteNoise(vec3i pos, u64 seed) {
	return hashedToFloat<F>(hash(BitCastVec2iToU64(pos.xy), (u64)pos.z, seed));
}

template <typename F>
inline F whiteNoise(vec4i pos, u64 seed) {
	return hashedToFloat<F>(hash(BitCastVec2iToU64(pos.xy), BitCastVec2iToU64(pos.zw), seed));
}

template <typename F>
F linearNoise(f64 x, u64 seed);

extern template
f32 linearNoise<f32>(f64 x, u64 seed);
extern template
f64 linearNoise<f64>(f64 x, u64 seed);

template <typename F>
F perlinNoise(f64 x, u64 seed);

extern template
f32 perlinNoise<f32>(f64 x, u64 seed);
extern template
f64 perlinNoise<f64>(f64 x, u64 seed);

template <typename F>
F perlinNoise(vec2d pos, u64 seed);

extern template
f32 perlinNoise<f32>(vec2d pos, u64 seed);
extern template
f64 perlinNoise<f64>(vec2d pos, u64 seed);

template <typename F>
F perlinNoise(vec2d pos, u64 seed, i32 nOctaves, F detail);

extern template
f32 perlinNoise<f32>(vec2d pos, u64 seed, i32 nOctaves, f32 detail);
extern template
f64 perlinNoise<f64>(vec2d pos, u64 seed, i32 nOctaves, f64 detail);


template <typename F>
F linearNoise(vec2d pos, u64 seed);

extern template
f32 linearNoise<f32>(vec2d x, u64 seed);
extern template
f64 linearNoise<f64>(vec2d x, u64 seed);

template <typename F>
F simplexNoise(vec2d pos, u64 seed);

extern template
f32 simplexNoise<f32>(vec2d x, u64 seed);
extern template
f64 simplexNoise<f64>(vec2d x, u64 seed);

template <typename F>
F simplexNoise(vec2d pos, u64 seed, i32 nOctaves, F detail);

extern template
f32 simplexNoise<f32>(vec2d pos, u64 seed, i32 nOctaves, f32 detail);
extern template
f64 simplexNoise<f64>(vec2d pos, u64 seed, i32 nOctaves, f64 detail);

template <typename F>
F cosineNoise(vec2d pos, u64 seed);

extern template
f32 cosineNoise<f32>(vec2d x, u64 seed);
extern template
f64 cosineNoise<f64>(vec2d x, u64 seed);

template <typename F>
F cubicNoise(vec2d pos, u64 seed);

extern template
f32 cubicNoise<f32>(vec2d x, u64 seed);
extern template
f64 cubicNoise<f64>(vec2d x, u64 seed);

} // namespace Noise

} // namespace AzCore

#endif // AZCORE_NOISE_HPP