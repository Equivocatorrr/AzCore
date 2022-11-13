/*
	File: Utilities.hpp
	Author: Philip Haynes
	Helpers for writing unit tests.
*/

#ifndef UTILITIES_HPP
#define UTILITIES_HPP

#include "AzCore/Memory/String.hpp"
#include "AzCore/Memory/Range.hpp"

template <typename FP>
struct FPError {
	i32 numTests = 0;
	FP sum = 0.0f;
	az::Array<FP> errors;
	FP errorMax = 0.0f;

	// Computes the error (Units in the Last Place) between the values,
	// reporting a problem when it goes above maxErrorWeak and maxErrorFail.
	// Also updates our stats.
	// magnitude represents the scale of our operations.
	// For example, when operating with unit vectors, magnitude would be 1.0
	void Compare(FP lhs, FP rhs, FP magnitude, i32 line, az::String info = az::String(), FP maxErrorWeak = FP(2), FP maxErrorFail = FP(100));

	// Reports our stats as Infos
	void Report(i32 line);
};

#endif // UTILITIES_HPP