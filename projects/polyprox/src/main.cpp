/*
	File: main.cpp
	Author: Philip Haynes
	Finds the coefficients of a polynomial to approximate a function.
*/

#include "AzCore/io.hpp"
#include "AzCore/math.hpp"
#include "AzCore/Memory/Array.hpp"

using namespace AzCore;

using Coefficients = Array<f64>;

f64 groundTruth(f64 x) {
	return sin(x);
}
f64 groundTruthDerivative(u32 nthDerivative, f64 x) {
	switch (nthDerivative % 4) {
		case 0: return sin(x);
		case 1: return cos(x);
		case 2: return -sin(x);
		case 3: return -cos(x);
		default: return 0.0; // Appease the compiler gods
	}
}

f64 eval(Coefficients coefficients, f64 x, f64 xMid) {
	f64 result = 0.0;
	x -= xMid;
	f64 xx = 1.0;
	for (f64 coefficient : coefficients) {
		result += xx * coefficient;
		xx *= x;
	}
	return result;
}

struct ErrorReport {
	f64 avg;
	f64 max;
	f64 min;
};

ErrorReport totalError(Coefficients coefficients, f64 xStart, f64 xMid, f64 xEnd) {
	ErrorReport result;
	result.avg = 0.0;
	result.max = -1.0;
	result.min = 100000.0;
	constexpr i32 NUM_TEST_POINTS = 1024 * 1024;
	constexpr f64 NUM_TEST_POINTS_D = NUM_TEST_POINTS;
	for (i32 i = 0; i < NUM_TEST_POINTS; i++) {
		f64 x = (f64)i / NUM_TEST_POINTS_D * (xEnd - xStart) + xStart;
		f64 value = eval(coefficients, x, xMid);
		f64 error = abs(value - groundTruth(x));
		result.avg += error;
		if (error > result.max) result.max = error;
		if (error < result.min) result.min = error;
	}
	result.avg /= NUM_TEST_POINTS_D;
	return result;
}

Coefficients FindCoefficients(i32 ord, f64 xStart, f64 xMid, f64 xEnd) {
	Coefficients coefficients(ord+1, 0.0);
	// Start with the taylor series
	coefficients[0] = groundTruth(xMid);
	f64 n = 1.0;
	f64 factorial = n;
	for (i32 i = 1; i <= ord; i++) {
		coefficients[i] = groundTruthDerivative(i, xMid) / factorial;
		n += 1.0;
		factorial *= n;
	}
	// Refine it for how many terms we have
#if 1
	for (i32 j = 0; j < 10000; j++) {
		for (i32 i = 0; i <= ord; i++) {
			f64 totalCoefficientError = 0.0;
			// Formula for integral of (x-halfpi64)^i
			// f64 coefficientScale1 = 1.0 / ((f64)(i+1) * pow(2.0, (f64)i)) * pow(abs(xEnd-xStart), (f64)(i));
			f64 coefficientScale2 = 0.0;
			constexpr i32 NUM_SAMPLES = 1024;
			constexpr f64 NUM_SAMPLES_D = NUM_SAMPLES;
			for (i32 xi = 0; xi <= NUM_SAMPLES; xi++) {
				f64 x = (f64)xi / NUM_SAMPLES_D * (xEnd-xStart) + xStart;
				f64 mag = pow(x-xMid, (f64)i);
				f64 pointError;
				pointError = groundTruth(x) - eval(coefficients, x, xMid);
				pointError *= mag;
				totalCoefficientError += pointError;
				coefficientScale2 += abs(mag) / NUM_SAMPLES_D;
			}
			coefficients[i] += totalCoefficientError / NUM_SAMPLES_D / coefficientScale2;
		}
	}
#endif
	return coefficients;
}

i32 main(i32 argumentCount, char** argumentValues) {

	f64 xMid = pi64/4.0;
	Coefficients coefficients;
	coefficients = FindCoefficients(7, 0.0, xMid, halfpi64);
	ErrorReport error = totalError(coefficients, 0.0, xMid, halfpi64);
	io::cout.PrintLn("Coefficients: ", Join(coefficients, ", "));
	io::cout.PrintLn("Error avg: ", FormatFloat(error.avg, 10, 3), ", max: ", FormatFloat(error.max, 10, 3), ", min: ", FormatFloat(error.min, 10, 3));

	return 0;
}
