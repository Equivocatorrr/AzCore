/*
	File: cli.hpp
	Author: Philip Haynes
	Helpers for parsing command line arguments.
*/

#ifndef AZCORE_CLI_HPP
#define AZCORE_CLI_HPP

#include "../Memory/String.hpp"
#include "../Memory/Array.hpp"
#include "../Memory/Range.hpp"

namespace AzCore::CLI {

// Gives an array of arguments, skipping the first one since it's just the program name.
inline Array<Str> GetArguments(i32 argc, char *argv[]) {
	Array<Str> out;
	out.Reserve(argc-1);
	for (i32 i = 1; i < argc; i++) {
		out.Append(argv[i]);
	}
	return out;
}

// TODO: Parsing for multi-argument flags

// TODO: TUI stuff

} // namespace AzCore::CLI

#endif // AZCORE_CLI_HPP