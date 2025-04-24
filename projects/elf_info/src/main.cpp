/*
	File: main.cpp
	Author: Philip Haynes
	Prints info about ELF binaries.
*/

#include "AzCore/elf/elf.hpp"
#include "AzCore/Utility/cli.hpp"

using namespace AzCore;

i32 main(i32 argc, char** argv) {
	// io::logLevel = io::LogLevel::TRACE;
	io::cout.IndentString("| ");
	Str path;
	bool programHeaders = false;
	bool sectionHeaders = false;
	bool dwarf_info = false;
	bool dwarf_abbrevs = false;
	cli::defs.flags = {
		{ 'p', "program", "Print info for each program header.",
			[&programHeaders](Str arg) {
				programHeaders = true;
				return false;
			}
		},
		{ 's', "section", "Print info for each section header.",
			[&sectionHeaders](Str arg) {
				sectionHeaders = true;
				return false;
			}
		},
		{ 'i', "dwarf-info", "Print info about DWARF .debug_info (DIEs)",
			[&dwarf_info](Str arg) {
				dwarf_info = true;
				return false;
			}
		},
		{ 'a', "dwarf-abbrevs", "Print info about DWARF .debug_abbrev (DIE specs)",
			[&dwarf_abbrevs](Str arg) {
				dwarf_abbrevs = true;
				return false;
			}
		},
	};
	cli::defs.defaultHandler = [&path](Str arg) {
		if (path.size) {
			io::cerr.PrintLn("Expected only one binary (got ", path, ", and then ", arg, ")...");
			return false;
		}
		path = arg;
		return true;
	};
	cli::defs.explanation = "path/to/elf_binary";
	if (!cli::ParseArguments(argc, argv)) {
		return 1;
	}
	if (path.size) {
		elf::File binary;
		if (auto result = binary.LoadFile(path); result.isError) {
			io::cerr.PrintLn("Error loading binary \"", path, "\": ", result.error);
			return 1;
		}
		binary.PrintHeaderInfo(io::cout, programHeaders, sectionHeaders);
		if (dwarf_info || dwarf_abbrevs) {
			binary.PrintDWARFInfo(io::cout, dwarf_abbrevs, dwarf_info);
		}
	} else {
		io::cerr.PrintLn("No path given");
		cli::PrintUsage();
	}
	return 0;
}
