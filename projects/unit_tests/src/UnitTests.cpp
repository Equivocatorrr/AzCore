#include "UnitTests.hpp"
#include "AzCore/IO/Log.hpp"
#include "AzCore/QuickSort.hpp"

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

void RunTests() {
	io::cout.PrintLn("Running ", allTests.size, " tests...");
	i32 testsRun = 0;
	i32 testsSucceeded = 0;
	i32 testsFailed = 0;
	i32 testsWeak = 0;
	for (TestInfo &test : allTests) {
		currentTestInfo = &test;
		io::cout.PrintLn("\nRunning \"", test.name, "\"");
		test.function();
		UT::EndTest();
		testsRun++;
		switch (test.result) {
			case Result::FAILURE: {
				io::cout.PrintLn("Test \"", test.name, "\" failed with ", test.problems.size, " problems.");
				testsFailed++;
			} break;
			case Result::WEAK: {
				io::cout.PrintLn("Test \"", test.name, "\" weak with ", test.problems.size, " problems.");
				testsWeak++;
			} break;
			case Result::SUCCESS: {
				io::cout.PrintLn("Test \"", test.name, "\" succeeded with ", test.problems.size, " problems.");
				testsSucceeded++;
			} break;
			default: {
				io::cout.PrintLn("Test \"", test.name, "\" doesn't have a valid result!");
			} break;
		}
		QuickSort(SimpleRange<Report>(test.problems.data, test.problems.size), [](Report lhs, Report rhs) { return lhs.line < rhs.line; });
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
			} else if (countLine >= 5) {
				skipCount++;
				continue;
			}
			io::cout.PrintLn("\t", problem.message);
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
				// if (skipCount) {
				// 	io::cout.PrintLn("Skipped ", skipCount, " infos from the same line.");
				// 	skipCount = 0;
				// }
				curLine = info.line;
				countLine = 0;
				io::cout.PrintLn("On line ", curLine);
			} /*else if (countLine >= 5) {
				skipCount++;
				continue;
			}*/
			io::cout.PrintLn("\t", info.message);
			countLine++;
		}
		// if (skipCount) {
		// 	io::cout.PrintLn("Skipped ", skipCount, " infos from the same line.");
		// 	skipCount = 0;
		// }
	}
	io::cout.PrintLn("Ran ", testsRun, " tests. ", testsSucceeded, " succeeded, ", testsFailed, " failed, and ", testsWeak, " were weak.");
}

} // namespace UT