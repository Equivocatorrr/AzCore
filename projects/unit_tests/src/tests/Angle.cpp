/*
	File: Angle.cpp
	Author: Philip Haynes
	Testing angle structs.
*/

#include "../UnitTests.hpp"
#include "../Utilities.hpp"
#include "AzCore/math.hpp"

namespace AngleTestNamespace {

using namespace AzCore;

void AngleTest();
UT::Register angleTest("Angle", AngleTest);

FPError<f32> fpError32;
FPError<f64> fpError64;

#define COMPARE_FP32(lhs, rhs, magnitude, info) fpError32.Compare(lhs, rhs, magnitude, __LINE__, info)
#define COMPARE_FP64(lhs, rhs, magnitude, info) fpError64.Compare(lhs, rhs, magnitude, __LINE__, info)

#define COMPARE_ANGLE32(lhs, rhs, magnitude, errorMult) fpError32.Compare(0.0f, (lhs-rhs).value(), magnitude, __LINE__, Stringify("lhs = ", lhs.value(), ", rhs = ", rhs.value()), errorMult, 10.0f * errorMult)
#define COMPARE_ANGLE64(lhs, rhs, magnitude, errorMult) fpError64.Compare(0.0,  (lhs-rhs).value(), magnitude, __LINE__, Stringify("lhs = ", lhs.value(), ", rhs = ", rhs.value()), errorMult, 10.0 * errorMult)

void AngleTest() {
	{ // f32 version
		Angle32 angle1, angle2;
		for (f32 baseAngle = 0.0f; baseAngle < tau; baseAngle += tau/360.0f) {
			angle1 = baseAngle;
			angle2 = angle1 + tau;
			COMPARE_ANGLE32(angle1, angle2, tau, 1.0f);
			angle2 = angle1 - tau;
			COMPARE_ANGLE32(angle1, angle2, tau, 1.0f);
			angle2 = angle1 + tau * 10.0f;
			COMPARE_ANGLE32(angle1, angle2, tau, 10.0f);
			angle2 = angle1 - tau * 10.0f;
			COMPARE_ANGLE32(angle1, angle2, tau, 10.0f);
			angle2 = angle1 + tau * 100.0f;
			COMPARE_ANGLE32(angle1, angle2, tau, 100.0f);
			angle2 = angle1 - tau * 100.0f;
			COMPARE_ANGLE32(angle1, angle2, tau, 100.0f);
		}
		angle1 = Degrees32(15.0f);
		angle2 = Radians32(halfpi/6.0f);
		COMPARE_ANGLE32(angle1, angle2, halfpi/6.0f, 1.0f);
		angle1 = Degrees32(22.5f);
		angle2 = Radians32(halfpi/4.0f);
		COMPARE_ANGLE32(angle1, angle2, halfpi/4.0f, 1.0f);
		angle1 = Degrees32(30.0f);
		angle2 = Radians32(halfpi/3.0f);
		COMPARE_ANGLE32(angle1, angle2, halfpi/3.0f, 1.0f);
		angle1 = Degrees32(45.0f);
		angle2 = Radians32(halfpi/2.0f);
		COMPARE_ANGLE32(angle1, angle2, halfpi/2.0f, 1.0f);
		angle1 = Degrees32(60.0f);
		angle2 = Radians32(halfpi*2.0f/3.0f);
		COMPARE_ANGLE32(angle1, angle2, halfpi*2.0f/3.0f, 1.0f);
		angle1 = Degrees32(90.0f);
		angle2 = Radians32(halfpi);
		COMPARE_ANGLE32(angle1, angle2, halfpi, 1.0f);
		angle1 = Degrees32(120.0f);
		angle2 = Radians32(pi*2.0f/3.0f);
		COMPARE_ANGLE32(angle1, angle2, pi*2.0f/3.0f, 1.0f);
		angle1 = Degrees32(180.0f);
		angle2 = Radians32(pi);
		COMPARE_ANGLE32(angle1, angle2, pi, 1.0f);
		angle1 = Degrees32(360.0f);
		angle2 = Radians32(tau);
		COMPARE_ANGLE32(angle1, angle2, tau, 1.0f);
	}
	{ // f64 version
		Angle64 angle1, angle2;
		for (f64 baseAngle = 0.0; baseAngle < tau64; baseAngle += tau64/360.0) {
			angle1 = baseAngle;
			angle2 = angle1 + tau64;
			COMPARE_ANGLE64(angle1, angle2, tau64, 1.0);
			angle2 = angle1 - tau64;
			COMPARE_ANGLE64(angle1, angle2, tau64, 1.0);
			angle2 = angle1 + tau64 * 10.0;
			COMPARE_ANGLE64(angle1, angle2, tau64, 10.0);
			angle2 = angle1 - tau64 * 10.0;
			COMPARE_ANGLE64(angle1, angle2, tau64, 10.0);
			angle2 = angle1 + tau64 * 100.0;
			COMPARE_ANGLE64(angle1, angle2, tau64, 100.0);
			angle2 = angle1 - tau64 * 100.0;
			COMPARE_ANGLE64(angle1, angle2, tau64, 100.0);
		}
		angle1 = Degrees64(15.0);
		angle2 = Radians64(halfpi64/6.0);
		COMPARE_ANGLE64(angle1, angle2, halfpi64/6.0, 1.0);
		angle1 = Degrees64(22.5);
		angle2 = Radians64(halfpi64/4.0);
		COMPARE_ANGLE64(angle1, angle2, halfpi64/4.0, 1.0);
		angle1 = Degrees64(30.0);
		angle2 = Radians64(halfpi64/3.0);
		COMPARE_ANGLE64(angle1, angle2, halfpi64/3.0, 1.0);
		angle1 = Degrees64(45.0);
		angle2 = Radians64(halfpi64/2.0);
		COMPARE_ANGLE64(angle1, angle2, halfpi64/2.0, 1.0);
		angle1 = Degrees64(60.0);
		angle2 = Radians64(halfpi64*2.0/3.0);
		COMPARE_ANGLE64(angle1, angle2, halfpi64*2.0/3.0, 1.0);
		angle1 = Degrees64(90.0);
		angle2 = Radians64(halfpi64);
		COMPARE_ANGLE64(angle1, angle2, halfpi64, 1.0);
		angle1 = Degrees64(120.0);
		angle2 = Radians64(pi64*2.0/3.0);
		COMPARE_ANGLE64(angle1, angle2, pi64*2.0/3.0, 1.0);
		angle1 = Degrees64(180.0);
		angle2 = Radians64(pi64);
		COMPARE_ANGLE64(angle1, angle2, pi64, 1.0);
		angle1 = Degrees64(360.0);
		angle2 = Radians64(tau64);
		COMPARE_ANGLE64(angle1, angle2, tau64, 1.0);
	}
	fpError32.Report(__LINE__);
	fpError64.Report(__LINE__);
}

} // namespace AngleTestNamespace