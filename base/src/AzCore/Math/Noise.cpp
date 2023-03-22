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

f32 hashedToF32(u64 x) {
	f32 result;
	// Largest value that can sit in the mantissa of an f32
	x &= 0x7fffff;
	result = (f32)x / (f32)0x7fffff;
	return result;
}

f64 hashedToF64(u64 x) {
	f64 result;
	// Largest value that can sit in the mantissa of an f64
	x &= 0xfffffffffffff;
	result = (f64)x / (f64)0xfffffffffffff;
	return result;
}

} // namespace Noise

} // namespace AzCore