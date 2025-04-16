/*
	File: main.cpp
	Author: Philip Haynes
	Prints info about ELF binaries.
*/

#include "AzCore/elf/elf.hpp"
#include "AzCore/Utility/cli.hpp"

using namespace AzCore;

void Usage(Str name) {
	io::cout.PrintLn(
		"Usage:\n$ ", name, " flags path/to/elf_binary"
		"\nFlags:"
		"\n\t-p --program    Print info for each program header."
		"\n\t-s --section    Print info for each section header."
	);
}

i32 main(i32 argc, char** argv) {
	Str name = argv[0];
	Array<Str> args = cli::GetArguments(argc, argv);
	Str path;
	bool programHeaders = false;
	bool sectionHeaders = false;
	for (Str &arg : args) {
		if (StartsWith(arg, "--")) {
			if (arg == "--program") {
				programHeaders = true;
			} else if (arg == "--section") {
				sectionHeaders = true;
			} else {
				io::cerr.PrintLn("Unknown flag ", arg);
				Usage(name);
				return 1;
			}
		} else if (StartsWith(arg, "-")) {
			for (i32 i = 1; i < arg.size; i++) {
				char c = arg[i];
				switch (c) {
					case 'p':
						programHeaders = true;
						break;
					case 's':
						sectionHeaders = true;
						break;
					default:
						io::cerr.PrintLn("Unknown flag ", c);
						Usage(name);
						return 1;
				}
			}
		} else {
			if (path.size) {
				io::cerr.PrintLn("Expected only one binary (got ", path, ", and then ", arg, ")...");
				Usage(name);
				return 1;
			}
			path = arg;
		}
	}
	if (path.size) {
		elf::File binary;
		if (auto result = binary.LoadFile(path); result.isError) {
			io::cerr.PrintLn("Error loading binary \"", path, "\": ", result.error);
			return 1;
		}
		binary.PrintHeaderInfo(io::cout, programHeaders, sectionHeaders);
	} else {
		Usage(name);
	}
	return 0;
}
