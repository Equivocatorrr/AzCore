#include "Utilities.hpp"
#include "UnitTests.hpp"

#include "AzCore/math.hpp"
#include "AzCore/IO/Log.hpp"
#include "AzCore/QuickSort.hpp"

template <typename FP>
void FPError<FP>::Compare(FP lhs, FP rhs, FP magnitude, i32 line, az::String info, FP maxErrorWeak, FP maxErrorFail) {
	FP error = abs(rhs - lhs) / (nextafter(magnitude, FP(HUGE_VAL)) - magnitude);
	errors.Append(error);
	numTests++;
	sum += error;
	if (error > errorMax) errorMax = error;
	if (error > maxErrorWeak) {
		UT::ReportProblem(line, error > maxErrorFail, "Comparing ", lhs, " and ", rhs, " yielded too much error (", error, "): \"", info, "\"");
		if (error > maxErrorFail) {
			UT::currentTestInfo->result = UT::Result::FAILURE;
		} else {
			if (UT::currentTestInfo->result == UT::Result::NOT_RUN_YET) {
				UT::currentTestInfo->result = UT::Result::WEAK;
			}
		}
	}
}

template <typename FP>
void FPError<FP>::Report(i32 line) {
	az::QuickSort(errors);
	f32 medianError = errors[errors.size/2];
	if ((errors.size % 2) == 0) {
		medianError += errors[errors.size/2 + 1];
		medianError /= FP(2);
	}
	UT::ReportInfo(line, "Number of FP Compares: ", numTests);
	UT::ReportInfo(line, "Median Error: ", az::FormatFloat(medianError, 10, 3));
	UT::ReportInfo(line, "Average Error: ", az::FormatFloat(sum / (FP)numTests, 10, 3));
	UT::ReportInfo(line, "Max Error: ", az::FormatFloat(errorMax, 10, 3));
}

template struct FPError<f32>;
template struct FPError<f64>;