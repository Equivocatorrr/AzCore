/*
    File: UnitTests.hpp
    Author: Philip Haynes
    Manager for unit tests.
*/

#ifndef UNIT_TESTS_HPP
#define UNIT_TESTS_HPP

#include "AzCore/Memory/Array.hpp"
#include "AzCore/Memory/String.hpp"

namespace UT {

using namespace AzCore;

enum class Result {
	NOT_RUN_YET = 0,
	SUCCESS, // For total success
	FAILURE, // For total failure
	WEAK,    // For partial failure, sub-optimal accuracy, etc.
};

typedef void (*fp_UnitTest)();

struct Report {
	String message;
	i32 line;
};

struct TestInfo {
	String name;
	Array<Report> problems;
	Array<Report> infos;
	fp_UnitTest function;
	Result result = Result::NOT_RUN_YET;
};

extern TestInfo *currentTestInfo;
struct Register {
	Register() = delete;
	Register(String name, fp_UnitTest function);
};
void RunTests();

template <typename... Args>
inline void ReportProblem(i32 line, Args... what) {
	currentTestInfo->problems.Append({Stringify(what...), line});
}

template <typename... Args>
inline void ReportInfo(i32 line, Args... what) {
	currentTestInfo->infos.Append({Stringify(what...), line});
}

} // namespace UT

// Use Assert if something must be true for the test to continue.
#define UTAssert(condition, ...) \
	if (!(condition)) { \
		UT::ReportProblem(__LINE__, "Assertion failed, aborting test: `", #condition, "`: ", ##__VA_ARGS__); \
		UT::currentTestInfo->result = UT::Result::FAILURE; \
		return; \
	}

// Use ExpectWeak if something can be wrong without completely failing the test.
#define UTExpectWeak(condition, ...) _UTExpect(condition, WEAK, ##__VA_ARGS__)
// Use Expect if something doesn't ruin the rest of the test.
#define UTExpect(condition, ...) _UTExpect(condition, FAILURE, ##__VA_ARGS__)
#define UTExpectEquals(lhs, rhs, ...) _UTExpect((lhs) == (rhs), FAILURE, "Expected ", #lhs, " to equal ", (rhs), ", but it was ", (lhs), ##__VA_ARGS__)
#define UTExpectEqualsWeak(lhs, rhs, ...) _UTExpect((lhs) == (rhs), WEAK, "Expected ", #lhs, " to equal ", (rhs), ", but it was ", (lhs), ##__VA_ARGS__)
#define _UTExpect(condition, RESULT, ...) \
	if (!(condition)) { \
		UT::ReportProblem(__LINE__, "Expectation not met: `", #condition, "`: ", ##__VA_ARGS__); \
		if (UT::currentTestInfo->result != UT::Result::FAILURE) \
			UT::currentTestInfo->result = UT::Result::RESULT; \
	}

#endif // UNIT_TESTS_HPP