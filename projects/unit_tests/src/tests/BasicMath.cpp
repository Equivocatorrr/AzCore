/*
	File: BasicMath.cpp
	Author: Philip Haynes
	Testing basic maths functions.
*/

#include "../UnitTests.hpp"
#include "../Utilities.hpp"
#include "AzCore/math.hpp"

namespace BasicMathTestNamespace {

using namespace AzCore;

void BasicMathTest();
UT::Register basicMath("BasicMath", BasicMathTest);

FPError<f32> fpError;

f32 maxErrorWeak = 10, maxErrorFail = 100;

#define COMPARE_FP(lhs, rhs, magnitude) fpError.Compare(lhs, rhs, magnitude, __LINE__, String(), maxErrorWeak, maxErrorFail)

#define COMPARE_VEC3(lhs, rhs, magnitude) \
	{ \
		COMPARE_FP(lhs.x, lhs.x, magnitude); \
		COMPARE_FP(lhs.y, lhs.y, magnitude); \
		COMPARE_FP(lhs.z, lhs.z, magnitude); \
	}

void BasicMathTest() {
	UTExpectEquals(max(2, 1), 2);
	UTExpectEquals(max(2, 2), 2);
	UTExpectEquals(min(2, 1), 1);
	UTExpectEquals(min(1, 1), 1);
	UTExpectEquals(max(-2, -1), -1);
	UTExpectEquals(max(-2, -2), -2);
	UTExpectEquals(min(-2, -1), -2);
	UTExpectEquals(min(-2, -2), -2);


	UTExpectEquals(median(1, 1, 1), 1);

	UTExpectEquals(median(1, 2, 3), 2);
	UTExpectEquals(median(3, 1, 2), 2);
	UTExpectEquals(median(2, 3, 1), 2);
	UTExpectEquals(median(1, 3, 2), 2);
	UTExpectEquals(median(3, 2, 1), 2);
	UTExpectEquals(median(2, 1, 3), 2);

	UTExpectEquals(median(1, 1, 2), 1);
	UTExpectEquals(median(1, 2, 1), 1);
	UTExpectEquals(median(2, 1, 1), 1);
	UTExpectEquals(median(2, 2, 1), 2);
	UTExpectEquals(median(2, 1, 2), 2);
	UTExpectEquals(median(1, 2, 2), 2);


	UTExpectEquals(clamp(1, 2, 3), 2);
	UTExpectEquals(clamp(2, 2, 3), 2);
	UTExpectEquals(clamp(3, 2, 3), 3);
	UTExpectEquals(clamp(4, 2, 3), 3);


	UTExpectEquals(abs(1), 1);
	UTExpectEquals(abs(0), 0);
	UTExpectEquals(abs(-1), 1);


	UTExpectEquals(sign(10), 1);
	UTExpectEquals(sign(0), 1);
	UTExpectEquals(sign(-10), -1);


	UTExpectEquals(lerp(0.0f, 1.0f, 0.5f), 0.5f);
	UTExpectEquals(lerp(1.0f, 2.0f, 0.5f), 1.5f);
	UTExpectEquals(lerp(1.0f, 3.0f, 0.5f), 2.0f);
	UTExpectEquals(lerp(1.0f, 5.0f, 0.25f), 2.0f);

	UTExpectEquals(lerp(1.0f, 0.0f, 0.5f), 0.5f);
	UTExpectEquals(lerp(2.0f, 1.0f, 0.5f), 1.5f);
	UTExpectEquals(lerp(3.0f, 1.0f, 0.5f), 2.0f);
	UTExpectEquals(lerp(5.0f, 1.0f, 0.75f), 2.0f);

	UTExpectEquals(lerp(-5.0f, 1.0f, 0.75f), -0.5f);


	UTExpectEquals(decay(1.0f, 0.0f, 1.0f, 1.0f), 0.5f);

	UTExpectEquals(map(1.0f, 0.5f, 1.5f, 2.0f, 4.0f), 3.0f);

	UTExpectEquals(cubert(27.0f), 3.0f);
	UTExpectEquals(cubert(-27.0f), -3.0f);

	COMPARE_VEC3(vec3(0.0f, 1.0f, 0.0f), orthogonalize(vec3(1.0f, 1.0f, 0.0f), vec3(1.0f, 0.0f, 0.0f)), 1.0f);
	COMPARE_FP(abs(dot(vec3(1.0f, 0.0f, 0.0f), orthogonalize(vec3(1.0f, 1.0f, 0.0f), vec3(1.0f, 0.0f, 0.0f)))), 0.0f, 1.0f);
	COMPARE_FP(abs(dot(vec3(1.0f, 0.0f, 0.0f), orthogonalize(vec3(1.0f, 0.0f, 0.0f), vec3(1.0f, 0.0f, 0.0f)))), 0.0f, 1.0f);
	COMPARE_FP(abs(dot(vec3(1.0f, 2.0f, 3.0f), orthogonalize(vec3(1.0f, 2.0f, 3.0f), vec3(1.0f, 2.0f, 3.0f)))), 0.0f, 4.0f);

	fpError.Report(__LINE__);
}

} // namespace BasicMathTestNamespace