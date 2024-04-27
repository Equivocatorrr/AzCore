/*
    File: Equations.cpp
    Author: Philip Haynes
*/

#include "AzCore/Math/Equations.hpp"
#include "../UnitTests.hpp"
#include "../Utilities.hpp"
#include "AzCore/Math/RandomNumberGenerator.hpp"
#include "AzCore/QuickSort.hpp"
#include "AzCore/memory.hpp"

namespace EquationsTestNamespace {

void EquationsTest();
UT::Register equationsTest("Equations", EquationsTest);

using namespace AzCore;

RandomNumberGenerator rng(69420);

using Real = f64;

FPError<Real> fpError;

Real maxErrorWeak, maxErrorFail;

#define COMPARE_FP(lhs, rhs, magnitude, info) fpError.Compare(lhs, rhs, magnitude, __LINE__, info, maxErrorWeak, maxErrorFail)

bool HasDuplicates(Array<Real> &array) {
	for (i32 i = 0; i < array.size; i++) {
		for (i32 j = i + 1; j < array.size; j++) {
			if (abs(array[i] - array[j]) / max(array[i], array[j]) < Real(0.001)) return true;
		}
	}
	return false;
}

void EquationsTest() {
	Array<Real> root;

	maxErrorWeak = 10;
	maxErrorFail = 100;
	root.Resize(2);
	for (i32 i = 0; i < 1000; i++) {
		root[0] = random(Real(-100), Real(100), &rng);
		root[1] = random(Real(-100), Real(100), &rng);
		if (root[0] > root[1]) {
			Swap(root[0], root[1]);
		}
		Real c0 = -root[0], c1 = -root[1];
		Real scale = random(Real(0.01), Real(100), &rng);
		if (HasDuplicates(root)) {
			--i;
			continue;
		}
		Real magnitude = max(abs(root[0]), abs(root[1])) * max(scale, Real(1));
		Real a = scale;
		Real b = scale * (c0 + c1);
		Real c = scale * (c0 * c1);
		magnitude = max(magnitude, abs(a), abs(b), abs(c));
		SolutionQuadratic solution = SolveQuadratic(a, b, c);
		UTExpectEqualsWeak(solution.nReal, 2,
			".  roots: ", root,
			"  a, b, c = ", a, ", ", b, ", ", c
		);
		if (solution.nReal != 2) continue;
		if (solution.root[0] > solution.root[1]) {
			Swap(solution.root[0], solution.root[1]);
		}
		COMPARE_FP(solution.root[0], root[0], magnitude, Stringify(
			"Actual Roots: ", root,
			"a, b, c = ", a, ", ", b, ", ", c
		));
		COMPARE_FP(solution.root[1], root[1], magnitude, Stringify(
			"Actual Roots: ", root,
			"a, b, c = ", a, ", ", b, ", ", c
		));
	}

	maxErrorWeak = 100;
	maxErrorFail = 1000;
	root.Resize(3);
	for (i32 i = 0; i < 1000; i++) {
		root[0] = random(Real(-100), Real(100), &rng);
		root[1] = random(Real(-100), Real(100), &rng);
		root[2] = random(Real(-100), Real(100), &rng);
		QuickSort(root);
		Real scale = random(Real(0.01), Real(100), &rng);
		if (HasDuplicates(root)) {
			--i;
			continue;
		}
		Real magnitude = max(abs(root[0]), abs(root[1]), abs(root[2])) * max(scale, Real(1));
		Real c0 = -root[0], c1 = -root[1], c2 = -root[2];
		Real a = scale;
		Real b = scale * (c0 + c1 + c2);
		Real c = scale * (c0 * c1 + c1 * c2 + c2 * c0);
		Real d = scale * (c0 * c1 * c2);
		magnitude = max(magnitude, abs(a), abs(b), abs(c), abs(d));
		SolutionCubic solution = SolveCubic(a, b, c, d);
		UTExpectEqualsWeak((i32)solution.nReal, 3,
			".  roots: ", root,
			"  a, b, c, d = ", a, ", ", b, ", ", c, ", ", d
		);
		if (solution.nReal != 3) continue;
		QuickSort(SimpleRange<Real>(solution.root, 3));
		COMPARE_FP(solution.root[0], root[0], magnitude, Stringify(
			"Actual Roots: ", root,
			"  a, b, c, d = ", a, ", ", b, ", ", c, ", ", d,
			"  magnitude = ", magnitude
		));
		COMPARE_FP(solution.root[1], root[1], magnitude, Stringify(
			"Actual Roots: ", root,
			"  a, b, c, d = ", a, ", ", b, ", ", c, ", ", d,
			"  magnitude = ", magnitude
		));
		COMPARE_FP(solution.root[2], root[2], magnitude, Stringify(
			"Actual Roots: ", root,
			"  a, b, c, d = ", a, ", ", b, ", ", c, ", ", d,
			"  magnitude = ", magnitude
		));
	}

	maxErrorWeak = 1000;
	maxErrorFail = 10000;
	root.Resize(4);
	for (i32 i = 0; i < 1000; i++) {
		root[0] = random(Real(-10), Real(10), &rng);
		root[1] = random(Real(-10), Real(10), &rng);
		root[2] = random(Real(-10), Real(10), &rng);
		root[3] = random(Real(-10), Real(10), &rng);
		QuickSort(root);
		Real scale = random(Real(0.1), Real(10), &rng);
		if (HasDuplicates(root)) {
			--i;
			continue;
		}
		Real magnitude = max(abs(root[0]), abs(root[1]), abs(root[2]), abs(root[3])) * max(scale, Real(1));
		Real c0 = -root[0], c1 = -root[1], c2 = -root[2], c3 = -root[3];
		Real a = scale;
		Real b = scale * (c0 + c1 + c2 + c3);
		Real c = scale * (c0 * c1 + c0 * c2 + c0 * c3 + c1 * c2 + c1 * c3 + c2 * c3);
		Real d = scale * (c0 * c1 * c2 + c0 * c1 * c3 + c0 * c2 * c3 + c1 * c2 * c3);
		Real e = scale * (c0 * c1 * c2 * c3);
		magnitude = max(magnitude, abs(a), abs(b), abs(c), abs(d), abs(e));
		SolutionQuartic solution = SolveQuartic(a, b, c, d, e);
		UTExpectEqualsWeak((i32)solution.nReal, 4,
			".  i = ", i,
			"  roots: ", root,
			"  a, b, c, d, e = ", a, ", ", b, ", ", c, ", ", d, ", ", e
		);
		if (solution.nReal != 4) continue;
		QuickSort(SimpleRange<Real>(solution.root, 4));
		COMPARE_FP(solution.root[0], root[0], magnitude, Stringify(
			"Actual Roots: ", root,
			"  a, b, c, d, e = ", a, ", ", b, ", ", c, ", ", d, ", ", e,
			"  magnitude = ", magnitude
		));
		COMPARE_FP(solution.root[1], root[1], magnitude, Stringify(
			"Actual Roots: ", root,
			"  a, b, c, d, e = ", a, ", ", b, ", ", c, ", ", d, ", ", e,
			"  magnitude = ", magnitude
		));
		COMPARE_FP(solution.root[2], root[2], magnitude, Stringify(
			"Actual Roots: ", root,
			"  a, b, c, d, e = ", a, ", ", b, ", ", c, ", ", d, ", ", e,
			"  magnitude = ", magnitude
		));
		COMPARE_FP(solution.root[3], root[3], magnitude, Stringify(
			"Actual Roots: ", root,
			"  a, b, c, d, e = ", a, ", ", b, ", ", c, ", ", d, ", ", e,
			"  magnitude = ", magnitude
		));
	}

	fpError.Report(__LINE__);
}

} // namespace EquationsTestNamespace