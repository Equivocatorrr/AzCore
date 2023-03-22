/*
	File: Noise.hpp
	Author: Philip Haynes
	Purely functional noise algorithms that output floating point values between 0 and 1.
*/

#ifndef AZCORE_NOISE_HPP
#define AZCORE_NOISE_HPP

#include "../basictypes.hpp"
#include "../math.hpp"

namespace AzCore {

namespace Noise {

u64 hash(u64 x);
u64 hash(u64 x, u64 y);
u64 hash(u64 x, u64 y, u64 z);
u64 hash(u64 x, u64 y, u64 z, u64 w);

// Converts the hash x to a f32 between 0 and 1
f32 hashedToF32(u64 x);
// Converts the hash x to a f64 between 0 and 1
f64 hashedToF64(u64 x);

// Meant for use with the following line:
// using namespace AzCore::Noise::Float32;
namespace Float32 {

inline f32 whiteNoise(u64 x) {
	return hashedToF32(hash(x));
}

inline f32 whiteNoise(u64 x, u64 seed) {
	return hashedToF32(hash(x, seed));
}

inline f32 whiteNoise(vec2i pos, u64 seed) {
	static_assert(sizeof(pos) == sizeof(u64));
	return hashedToF32(hash(*((u64*)&pos), seed));
}

inline f32 whiteNoise(vec3i pos, u64 seed) {
	static_assert(sizeof(pos.xy) == sizeof(u64));
	return hashedToF32(hash(*((u64*)&pos.xy), (u64)pos.z, seed));
}

inline f32 whiteNoise(vec4i pos, u64 seed) {
	static_assert(sizeof(pos.xy) == sizeof(u64));
	static_assert(sizeof(pos.zw) == sizeof(u64));
	return hashedToF32(hash(*((u64*)&pos.xy), *((u64*)&pos.zw), seed));
}

} // namespace Float32

// Meant for use with the following line:
// using namespace AzCore::Noise::Float64;
namespace Float64 {

inline f64 whiteNoise(u64 x) {
	return hashedToF64(hash(x));
}

inline f64 whiteNoise(u64 x, u64 seed) {
	return hashedToF64(hash(x, seed));
}

inline f64 whiteNoise(vec2i pos, u64 seed) {
	static_assert(sizeof(pos) == sizeof(u64));
	return hashedToF64(hash(*((u64*)&pos), seed));
}

inline f64 whiteNoise(vec3i pos, u64 seed) {
	static_assert(sizeof(pos.xy) == sizeof(u64));
	return hashedToF64(hash(*((u64*)&pos.xy), (u64)pos.z, seed));
}

inline f64 whiteNoise(vec4i pos, u64 seed) {
	static_assert(sizeof(pos.xy) == sizeof(u64));
	static_assert(sizeof(pos.zw) == sizeof(u64));
	return hashedToF64(hash(*((u64*)&pos.xy), *((u64*)&pos.zw), seed));
}

} // namespace Float64

} // namespace Noise

} // namespace AzCore

#endif // AZCORE_NOISE_HPP