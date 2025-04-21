#include "UnitTests.hpp"
#include "AzCore/IO/Log.hpp"
#include "AzCore/Utility/Sort.hpp"
#include "AzCore/IO/vt_strings.hpp"

namespace UT {

TestInfo *currentTestInfo = nullptr;

Array<TestInfo> allTests;

Register::Register(String name, fp_UnitTest function) {
	TestInfo test;
	test.name = name;
	test.function = function;
	allTests.Append(test);
}

void EndTest() {
	if (currentTestInfo->result == Result::NOT_RUN_YET) {
		currentTestInfo->result = Result::SUCCESS;
	}
}

void ListAllTests() {
	io::cout.PrintLn("The available tests are:");
	io::cout.IndentMore();
	for (TestInfo &test : allTests) {
		// TODO: Add explanations of all the tests.
		io::cout.PrintLn(test.name);
	}
	io::cout.IndentLess();
}

void RunTests(const Array<Str> &tests, i32 failLimit, i32 weakLimit, i32 infoLimit) {
	if (failLimit <= 0) failLimit = INT32_MAX;
	if (weakLimit <= 0) weakLimit = INT32_MAX;
	if (infoLimit <= 0) infoLimit = INT32_MAX;
	io::cout.PrintLn("Running ", tests.size ? tests.size : allTests.size, " tests...");
	i32 testsRun = 0;
	i32 testsSucceeded = 0;
	Array<Str> testsFailed;
	Array<Str> testsWeak;
	for (TestInfo &test : allTests) {
		if (tests.size && !tests.Contains(test.name)) {
			continue;
		}
		currentTestInfo = &test;
		io::cout.PrintLn("\nRunning \"", test.name, "\"");
		test.function();
		UT::EndTest();
		testsRun++;
		switch (test.result) {
			case Result::FAILURE: {
				io::cout.PrintLn("Test \"", test.name, "\" failed with ", test.problems.size, " problems.");
				testsFailed.Append(test.name);
			} break;
			case Result::WEAK: {
				io::cout.PrintLn("Test \"", test.name, "\" weak with ", test.problems.size, " problems.");
				testsWeak.Append(test.name);
			} break;
			case Result::SUCCESS: {
				io::cout.PrintLn("Test \"", test.name, "\" succeeded with ", test.problems.size, " problems.");
				testsSucceeded++;
			} break;
			default: {
				io::cout.PrintLn("Test \"", test.name, "\" doesn't have a valid result!");
			} break;
		}
		Sort(test.problems, [](Array<Report> &array, i64 indexLHS, i64 indexRHS) { return array[indexLHS].line < array[indexRHS].line; });
		i32 curLine = 0;
		i32 countLine = 0;
		i32 skipCount = 0;
		for (Report &problem : test.problems) {
			if (problem.line > curLine) {
				if (skipCount) {
					io::cout.PrintLn("Skipped ", skipCount, " problems from the same line.");
					skipCount = 0;
				}
				curLine = problem.line;
				countLine = 0;
				io::cout.PrintLn("On line ", curLine);
			} else if (countLine >= (problem.fail ? failLimit : weakLimit)) {
				skipCount++;
				continue;
			}
			if (problem.fail) {
				io::cout.PrintLn("\t", vt_span(VT_FG_RED, problem.message));
			} else {
				io::cout.PrintLn("\t", vt_span(VT_FG_YELLOW, problem.message));
			}
			countLine++;
		}
		if (skipCount) {
			io::cout.PrintLn("Skipped ", skipCount, " problems from the same line.");
			skipCount = 0;
		}
		if (test.infos.size != 0) {
			io::cout.PrintLn("Also had ", test.infos.size, " infos:");
		}
		curLine = 0;
		countLine = 0;
		for (Report &info : test.infos) {
			if (info.line > curLine) {
				if (skipCount) {
					io::cout.PrintLn("Skipped ", skipCount, " infos from the same line.");
					skipCount = 0;
				}
				curLine = info.line;
				countLine = 0;
				io::cout.PrintLn("On line ", curLine);
			} else if (countLine >= infoLimit) {
				skipCount++;
				continue;
			}
			io::cout.PrintLn("\t", vt_span(VT_FG_BLUE, info.message));
			countLine++;
		}
		if (skipCount) {
			io::cout.PrintLn("Skipped ", skipCount, " infos from the same line.");
			skipCount = 0;
		}
	}
	io::cout.PrintLn(testsSucceeded, "/", testsRun, " succeeded. ", testsFailed.size, " failed, and ", testsWeak.size, " were weak.");
	if (testsFailed.size) {
		io::cout.PrintLn("Failed tests: ", testsFailed);
	}
	if (testsWeak.size) {
		io::cout.PrintLn("Weak tests: ", testsWeak);
	}
}

} // namespace UT