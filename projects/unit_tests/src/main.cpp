/*
	File: main.cpp
	Author: Philip Haynes
	Taking unit testing into the 21st century.
*/

#include "UnitTests.hpp"
#include "AzCore/Utility/cli.hpp"

using namespace AzCore;

int main(int argc, char **argv) {
	bool names = false;
	Str test;
	i32 failLimit = 5;
	i32 weakLimit = 5;
	i32 infoLimit = 0;
	cli::defs.flags = {
		{ 'h', "help", "Print this help", [](Str arg) { cli::PrintUsage(); return false; } },
		{ 'n', "names", "List the name of each test", [&names](Str arg) { names = true; return false; } },
		{ 't', "test", "Choose a specific test to run", [&test](Str arg) { test = arg; return true; } },
		{ 'f', "fail-limit", "Specify how many 'fail' reports to show from one line (where 0 means no limit)",
			[&failLimit](Str arg) {
				if (!StringToI32(arg, &failLimit)) {
					io::cerr.PrintLn("Invalid limit count \"", arg, "\", ignoring...");
				}
				return true;
			}
		},
		{ 'w', "weak-limit", "Specify how many 'weak' reports to show from one line (where 0 means no limit)",
			[&weakLimit](Str arg) {
				if (!StringToI32(arg, &weakLimit)) {
					io::cerr.PrintLn("Invalid limit count \"", arg, "\", ignoring...");
				}
				return true;
			}
		},
		{ 'i', "info-limit", "Specify how many 'info' reports to show from one line (where 0 means no limit)",
			[&infoLimit](Str arg) {
				if (!StringToI32(arg, &infoLimit)) {
					io::cerr.PrintLn("Invalid limit count \"", arg, "\", ignoring...");
				}
				return true;
			}
		},
		{ 'a', "all", "Show all reports without limits",
			[&](Str arg) {
				failLimit = 0; weakLimit = 0; infoLimit = 0;
				return false;
			}
		},
	};
	if (!cli::ParseArguments(argc, argv)) {
		return 1;
	}
	if (names) {
		UT::ListAllTests();
		return 0;
	}
	UT::RunTests(test.size ? Array<Str>{test} : Array<Str>{}, failLimit, weakLimit, infoLimit);
	return 0;
}