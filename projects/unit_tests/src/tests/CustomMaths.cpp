/*
	File: CustomMaths.cpp
	Author: Philip Haynes
	Verifying the effectiveness and error margins of custom math approximation functions.
*/

#include "AzCore/Math/FastApprox.hpp"
#include "../UnitTests.hpp"
#include "../Utilities.hpp"

namespace CustomMathsTestNamespace {

void CustomMathsTest();
UT::Register customMathsTest("CustomMaths", CustomMathsTest);

using namespace AzCore;

using Real = f64;

FPError<Real> fpError;

Real maxErrorWeak = 100, maxErrorFail = 1000;

#define COMPARE_FP(lhs, rhs, magnitude, info) fpError.Compare(lhs, rhs, magnitude, __LINE__, info, maxErrorWeak, maxErrorFail)

void CustomMathsTest() {
	constexpr i32 NUM_SAMPLES = 100000;
	constexpr Real NUM_SAMPLES_R = NUM_SAMPLES;
	for (i32 i = 0; i < NUM_SAMPLES; i++) {
		Real x = (Real)i / NUM_SAMPLES_R * Real(4.0 * tau64) - Real(2.0 * tau64);
		COMPARE_FP(fa::sin<Real>(x), sin(x), 1.0f, Stringify("x = ", x));
	}
	
	fpError.Report(__LINE__);
}

} // namespace CustomMathsTestNamespace