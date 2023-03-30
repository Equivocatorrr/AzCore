/*
	File: FastApprox.hpp
	Author: Philip Haynes
	Approximations of common maths functions.
*/

#ifndef AZCORE_MATH_FAST_APPROX
#define AZCORE_MATH_FAST_APPROX

#include "basic.hpp"

namespace AzCore {

namespace FastApprox {

template <typename T>
T sin(T x) {
	// Taylor series modified to minimize error with the given number of terms and range.
	// The polynomial is centered around pi/4
	// Valid in the range 0 to pi/2
	// Avg error: 1.016e-8
	// Max error: 7.278e-8
	static const T coefficients[] = {
		T(0.70710677437360185),
		T(0.70710679442143958),
		T(-0.35355280824457847),
		T(-0.11785124129540452),
		T(0.029456667455724185),
		T(5.8924436437551877e-3),
		T(-9.6321296106989878e-4),
		T(-1.38728121719408e-4),
	};
	// Put x into the range 0 to tau
	x = wrap(x, T(tau64));
	// Second half of wrapped range is the same as the first but negated
	T outputSign = -sign(x - T(pi64));
	// Put x into the range 0 to pi
	x -= T(pi64) * T(x >= T(pi64));
	// Mirror around pi/2, putting x into the range 0 to pi/2
	x = x * T(x < T(pi64/2.0)) + (T(pi64) - x) * T(x >= T(pi64/2.0));
	x -= T(pi64/4.0);
	T result = coefficients[0];
	T xx = x;
	for (i32 i = 1; i < sizeof(coefficients) / sizeof(T); i++) {
		result += xx * coefficients[i];
		xx *= x;
	}
	result *= outputSign;
	return result;
}

} // namespace FastApprox

namespace fa = FastApprox;

} // namespace AzCore

#endif // AZCORE_MATH_FAST_APPROX