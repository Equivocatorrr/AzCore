/*
	File: basic.cpp
	Author: Philip Haynes
*/

#include "basic.hpp"

#include "../Memory/Util.hpp"

namespace AzCore {

f32 Power(f32 base, f32 exponent) {
	f32 a = base/(1+base);
	f32 b = 1.0 - a;
	bool swapFracs = false;
	if (exponent < 0.0) {
		base = 1.0/base;
		exponent = -exponent;
		swapFracs = true;
	} else if (exponent == 0.0) {
		return 1.0;
	}
	u32 exp = (u32)exponent;
	f32 expFrac = exponent - (f32)exp;
	// Reasonably map 0 to 1 into (0 to 1)^2
	expFrac = a*expFrac + b*expFrac*expFrac;
	f32 invExpFrac = 1.0 - expFrac;
	if (swapFracs) {
		f32 tmp = expFrac;
		expFrac = invExpFrac;
		invExpFrac = tmp;
	}
	f32 result1 = 1.0;
	if (exp == 0) result1 /= base;
	while (exp > 1) {
		if (exp & 1) {
			// odd
			result1 *= base;
			base *= base;
			exp = (exp-1) / 2;
		} else {
			// even
			base *= base;
			exp /= 2;
		}
	}
	result1 *= base;
	f32 result2 = result1 * base;
	return invExpFrac * result1 + expFrac * result2;
}

u64 GreatestCommonFactor(u64 a, u64 b) {
	if (a == 0) return b;
	if (b == 0) return a;
	u32 shift = CountTrailingZeroBits(a | b);
	a >>= CountTrailingZeroBits(a);
	do {
		b >>= CountTrailingZeroBits(b);
		if (a > b) {
			Swap(a, b);
		}
		b -= a;
	} while (b != 0);
	return a << shift;
}

u64 GreatestCommonFactor(std::initializer_list<u64> list) {
	auto it = list.begin();
	u64 val = *it++;
	while (it != list.end()) {
		val = GreatestCommonFactor(val, *it++);
	}
	return val;
}

// static long long greatest_common_divisor(long long a, long long b) {
// 	// while (a != b) {
// 	//     if (a > b) {
// 	//         a -= b;
// 	//     } else {
// 	//         b -= a;
// 	//     }
// 	// }
// 	// return a;
// 	int shift;
// 	if (a == 0) return b;
// 	if (b == 0) return a;
// 	shift = builtin_ctzll(a | b);
// 	a >>= builtin_ctzll(a);
// 	do {
// 		b >>= __builtin_ctzll(b);
// 		if (a > b) {
// 			long long t = b;
// 			b = a;
// 			a = t;
// 		}
// 		b = b - a;
// 	} while (b != 0);
// 	return a << shift;
// }

} // namespace AzCore
