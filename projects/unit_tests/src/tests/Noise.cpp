/*
	File: Noise.cpp
	Author: Philip Haynes
	Rigor testing for noise functions.
*/

#include "AzCore/Math/Noise.hpp"
#include "../UnitTests.hpp"
#include "../Utilities.hpp"

namespace NoiseTestNamespace {

using namespace AzCore;

void NoiseTest();
UT::Register noiseTest("Noise", NoiseTest);

FPError<f32> error;

// Just some uniform distribution tests
void NoiseTest() {
	constexpr i64 NUM = (i64)1<<26;
	f64 mean = 0.0;
	f64 variance = 0.0;
	for (i64 i = 0; i < NUM; i++) {
		mean += Noise::Float32::whiteNoise(i);
	}
	mean /= NUM;
	for (i64 i = 0; i < NUM; i++) {
		variance += square(Noise::Float32::whiteNoise(i) - mean);
	}
	variance /= NUM;
	UT::ReportInfo(__LINE__, "mean: ", FormatFloat(mean, 10, 3), ", variance: ", FormatFloat(variance, 10, 3));
	UTExpect(abs(mean-0.5) < 0.001);
	UTExpect(abs(variance-1.0/12.0) < 0.001);
	mean = 0.0;
	variance = 0.0;
	for (i64 i = 0; i < NUM; i++) {
		mean += Noise::Float64::whiteNoise(i);
	}
	mean /= NUM;
	for (i64 i = 0; i < NUM; i++) {
		variance += square(Noise::Float64::whiteNoise(i) - mean);
	}
	variance /= NUM;
	UT::ReportInfo(__LINE__, "mean: ", FormatFloat(mean, 10, 3), ", variance: ", FormatFloat(variance, 10, 3));
	UTExpect(abs(mean-0.5) < 0.001);
	UTExpect(abs(variance-1.0/12.0) < 0.001);
}

} // namespace NoiseTestNamespace