/*
    File: RandomNumberGenerator.cpp
    Author: Philip Haynes
    If anyone understands probabilities better than Philip (likely), please make this make sense.
*/

#include "AzCore/Math/RandomNumberGenerator.hpp"
#include "../UnitTests.hpp"
#include "../Utilities.hpp"
#include "AzCore/Memory/Array.hpp"
#include "AzCore/Memory/BinarySet.hpp"

namespace RandomNumberGeneratorTestNamespace {

using namespace AzCore;

void RandomNumberGeneratorTest();
UT::Register randomNumberGeneratorTest("RandomNumberGenerator", RandomNumberGeneratorTest);

const i32 INT_RANGE = 10000;

/* Cumulative Normal Function by Graeme West
   https://s2.smu.edu/~aleskovs/emis/sqc2/accuratecumnorm.pdf
   TODO: Probably move this into AzCore base (Do we want to make a stats library?). */
f64 CumulativeNorm(f64 x) {
	f64 result;
	f64 xAbs = abs(x);
	if (xAbs > 37.0) {
		result = 0.0;
	} else {
		f64 exponential = exp(-square(xAbs) / 2.0);
		f64 build;
		if (xAbs < 7.07106781186547) {
			build = 3.52624965998911e-2 * xAbs + 0.700383064443688;
			build = build * xAbs + 6.37396220353165;
			build = build * xAbs + 33.912866078383;
			build = build * xAbs + 112.079291497871;
			build = build * xAbs + 221.213596169931;
			build = build * xAbs + 220.206867912376;
			result = exponential * build;
			build = 8.83883476483184e-2 * xAbs + 1.75566716318264;
			build = build * xAbs + 16.064177579207;
			build = build * xAbs + 86.7807322029461;
			build = build * xAbs + 296.564248779674;
			build = build * xAbs + 637.333633378831;
			build = build * xAbs + 793.826512519948;
			build = build * xAbs + 440.413735824752;
			result /= build;
		} else {
			build = xAbs + 0.65;
			build = xAbs + 4.0 / build;
			build = xAbs + 3.0 / build;
			build = xAbs + 2.0 / build;
			build = xAbs + 1.0 / build;
			result = exponential / build / 2.506628274631;
		}
	}
	if (x > 0) result = 1.0 - result;
	return result;
}

i64 factorial(i64 n) {
	i64 result = 1;
	while (n > 1) {
		result *= n;
		n--;
	}
	return result;
}

f64 logFactorial(i32 n) {
	f64 result = 0;
	while (n > 1) {
		result += log2((f64)n);
		n--;
	}
	return result;
}

i32 nCr(i32 n, i32 r) {
	AzAssert(n > r, "n must be > r");
	return factorial(n) / (factorial(n - r) * factorial(r));
}

f64 lognCr(i32 n, i32 r) {
	AzAssert(n > r, "n must be > r");
	return logFactorial(n) - (logFactorial(n - r) + logFactorial(r));
}

f64 probOfOccurrence(i32 times, i32 tries, f64 logProb, f64 logInvProb) {
	return pow(2, lognCr(tries, times) + logProb * times + logInvProb * (tries - times));
}

struct BadGenerator {
	u32 x = 69420;
	u32 Generate() {
		x ^= x << 5;
		// x ^= x >> 7;
		// x ^= x << 22;
		return x;
	}
};

f64 GetMean(const Array<i32> &array) {
	f64 mean = 0.0;
	for (i32 i = 0; i < array.size; i++) {
		mean += array[i];
	}
	mean /= array.size;
	return mean;
}

f64 GetVariance(const Array<i32> &array, f64 mean) {
	f64 variance = 0.0;
	for (i32 i = 0; i < array.size; i++) {
		variance += square((f64)array[i] - mean);
	}
	variance /= array.size;
	return variance;
}

void RandomNumberGeneratorTest() {
	{ // Integer Test
		RandomNumberGenerator rng(69420);
		BadGenerator badRNG;
		Array<i32> duplicity(INT_RANGE, 0);
		i32 dups = 0;
		for (i32 i = 0; i < INT_RANGE; i++) {
			i32 val = random(0, INT_RANGE - 1, &rng);
			// i32 val = badRNG.Generate() % INT_RANGE;
			duplicity[val]++;
			if (duplicity[val] == 2) {
				dups++;
			}
		}
		f64 logProbOfBeingHit = log2(1.0 / f64(INT_RANGE));
		f64 logProbOfNotBeingHit = log2(f64(INT_RANGE - 1) / f64(INT_RANGE));

		f64 expectedDups = 0;
		for (i32 i = 2; i < sqrt(INT_RANGE); i++) {
			f64 probOfBeingHitITimes = probOfOccurrence(i, INT_RANGE, logProbOfBeingHit, logProbOfNotBeingHit);
			expectedDups += probOfBeingHitITimes;
		}
		expectedDups *= f64(INT_RANGE);
		UTExpect(abs(dups - (i32)expectedDups) < (i32)sqrt(INT_RANGE),
			"Expected approximately ", (i32)expectedDups, " duplicates, but got ", dups
		);
		Array<i32> duplicities(10, 0);
		for (i32 i = 0; i < INT_RANGE; i++) {
			if (duplicity[i] >= duplicities.size) {
				duplicities.Resize(duplicity[i] + 1, 0);
			}
			duplicities[duplicity[i]]++;
		}

		// variance of the binomial distribution representing our probabilities
		f64 variance = (f64)INT_RANGE * pow(2, logProbOfBeingHit) * pow(2, logProbOfNotBeingHit);
		f64 stdDev = sqrt(variance);
		UT::ReportInfo(__LINE__, "variance: ", FormatFloat(variance, 10, 6), ", stdDev: ", FormatFloat(stdDev, 10, 6));

		for (i32 i = 0; i < duplicities.size; i++) {
			f64 expectedDuplicityProb = probOfOccurrence(i, INT_RANGE, logProbOfBeingHit, logProbOfNotBeingHit);
			f64 observedDuplicityProb = (f64)duplicities[i] / (f64)INT_RANGE;
			i32 expectedDuplicity = expectedDuplicityProb * (f64)INT_RANGE + 0.5;

			if (duplicities[i] == 0 && expectedDuplicity == 0) continue;

			// NOTE: This is probably wrong, but I'm not sure what would be better.
			f64 zScore = (observedDuplicityProb - expectedDuplicityProb) / stdDev * sqrt((f64)INT_RANGE);
			// NOTE: Does this being a 2-tailed test make sense?
			f64 pValue = CumulativeNorm(-abs(zScore)) * 2.0;
			UTExpect(pValue > 0.2,
				"pValue <= 0.2 means our randomness isn't good enough. pValue was ", FormatFloat(pValue, 10, 2),
				"  expected ", expectedDuplicity, " numbers with duplicity ", i, " but we had ", duplicities[i]
			);
			UT::ReportInfo(__LINE__,
				"Numbers with duplicity ", i, ": ", duplicities[i],
				", expected ", expectedDuplicity,
				", zScore = ", FormatFloat(zScore, 10, 2),
				", pValue = ", FormatFloat(pValue, 10, 2)
			);
		}
	}

	{ // f32 distribution test
		constexpr i32 NUM_BINS = 10000;
		constexpr i32 NUM_SAMPLES_PER_BIN = 100;
		constexpr f64 NUM_BINS_F = NUM_BINS;
		constexpr f64 NUM_SAMPLES_PER_BIN_F = NUM_SAMPLES_PER_BIN;
		RandomNumberGenerator rng(69420);
		Array<i32> distribution(NUM_BINS, 0);
		for (i32 i = 0; i < NUM_BINS * NUM_SAMPLES_PER_BIN; i++) {
			f32 value = random(0.0f, (f32)NUM_BINS_F, &rng);
			UTAssert(value >= 0.0f && value <= NUM_BINS_F);
			// Squash our exact max value for binning.
			i32 bin = min((i32)floor(value), NUM_BINS-1);
			distribution[bin]++;
		}
		f64 mean = GetMean(distribution);
		UTExpect(abs(NUM_SAMPLES_PER_BIN_F - mean) / NUM_SAMPLES_PER_BIN_F < 0.01, "mean of ", mean, " is too far off from ", NUM_SAMPLES_PER_BIN_F);
		f64 variance = GetVariance(distribution, mean);
		f64 stdDev = sqrt(variance);
		f64 expectedVariance = NUM_BINS_F * NUM_SAMPLES_PER_BIN_F * (1.0/NUM_BINS_F) * (1.0 - 1.0/NUM_BINS_F);
		f64 expectedStdDev = sqrt(expectedVariance);
		UT::ReportInfo(__LINE__, "f32 distribution mean: ", mean, ", variance: ", variance, " (expected ", expectedVariance, "), std dev: ", stdDev, " (expected ", expectedStdDev, ")");
		UTExpect(abs(stdDev - expectedStdDev)/expectedStdDev < 0.01);
	}
	{ // f64 distribution test
		constexpr i32 NUM_BINS = 10000;
		constexpr i32 NUM_SAMPLES_PER_BIN = 100;
		constexpr f64 NUM_BINS_F = NUM_BINS;
		constexpr f64 NUM_SAMPLES_PER_BIN_F = NUM_SAMPLES_PER_BIN;
		RandomNumberGenerator rng(69420);
		Array<i32> distribution(NUM_BINS, 0);
		for (i32 i = 0; i < NUM_BINS * NUM_SAMPLES_PER_BIN; i++) {
			f64 value = random(0.0, NUM_BINS_F, &rng);
			UTAssert(value >= 0.0f && value <= NUM_BINS_F);
			// Squash our exact max value for binning.
			i32 bin = min((i32)floor(value), NUM_BINS-1);
			distribution[bin]++;
		}
		f64 mean = GetMean(distribution);
		UTExpect(abs(NUM_SAMPLES_PER_BIN_F - mean) / NUM_SAMPLES_PER_BIN_F < 0.01, "mean of ", mean, " is too far off from ", NUM_SAMPLES_PER_BIN_F);
		f64 variance = GetVariance(distribution, mean);
		f64 stdDev = sqrt(variance);
		f64 expectedVariance = NUM_BINS_F * NUM_SAMPLES_PER_BIN_F * (1.0/NUM_BINS_F) * (1.0 - 1.0/NUM_BINS_F);
		f64 expectedStdDev = sqrt(expectedVariance);
		UT::ReportInfo(__LINE__, "f64 distribution mean: ", mean, ", variance: ", variance, " (expected ", expectedVariance, "), std dev: ", stdDev, " (expected ", expectedStdDev, ")");
		UTExpect(abs(stdDev - expectedStdDev)/expectedStdDev < 0.01);
	}
}

} // namespace RandomNumberGeneratorTestNamespace