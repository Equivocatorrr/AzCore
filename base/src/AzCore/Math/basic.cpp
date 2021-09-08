/*
	File: basic.cpp
	Author: Philip Haynes
*/

#include "basic.hpp"

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

} // namespace AzCore
