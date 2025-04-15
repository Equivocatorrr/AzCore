/*
	File: main.cpp
	Author: Philip Haynes
	Prints info about ELF binaries.
*/

#include "AzCore/elf/elf.hpp"
#include "AzCore/Utility/cli.hpp"

using namespace AzCore;

void Usage(Str name) {
	io::cout.PrintLn("Usage:\n$ ", name, " path/to/elf_binary");
}

i32 main(i32 argc, char** argv) {
	Str name = argv[0];
	Array<Str> args = cli::GetArguments(argc, argv);
	if (args.size == 1) {
		elf::File binary;
		if (auto result = binary.LoadFile(args[0]); result.isError) {
			io::cerr.PrintLn("Error loading binary \"", args[0], "\": ", result.error);
			return 1;
		}
		binary.PrintHeaderInfo();
	} else {
		Usage(name);
	}
	return 0;
}
