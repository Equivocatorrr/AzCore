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
#include "../Memory/Optional.hpp"
#include "../IO/Log.hpp"
#include "../Math/Basic.hpp"
#include <functional>

namespace AzCore::cli {

// Gives an array of arguments, skipping the first one since it's just the program name.
inline Array<Str> GetArguments(i32 argc, char *argv[]) {
	Array<Str> out;
	out.Reserve(argc-1);
	for (i32 i = 1; i < argc; i++) {
		out.Append(argv[i]);
	}
	return out;
}

using ArgHandler_t = std::function<bool(Str)>;

struct Flag {
	// Represents a single-letter flag which can be found in a sequence of the form "-dab"
	char letter;
	// Represents a single-word flag name of the form "flag-name".
	// This will match an argument of the form "--flag-name".
	Str name;
	// Explanation of what this flag does, used when generating a Usage function.
	Str explanation;
	// Callback for handling what to do when this flag gets encountered. Probably should be a lambda that captures some associated value by reference.
	// should return true if the Str argument given was consumed
	ArgHandler_t parseCallback;
};

struct Definitions {
	Str programName;
	Array<Flag> flags;
	Str explanation;
	ArgHandler_t defaultHandler;
};
extern Definitions defs;

inline void PrintUsage(const Str &programName=defs.programName, const Array<Flag> &flags=defs.flags, const Str &explanation=defs.explanation) {
	io::cout.PrintLn("Usage:\n$ ", programName, flags.size ? " flags " : " ", explanation);
	if (flags.size) {
		io::cout.PrintLn("Flags:");
	}
	io::cout.IndentMore();
	i32 alignment = 0;
	for (const Flag &flag : flags) {
		i32 myAlignment = 0;
		if (flag.letter) {
			myAlignment += 4;
		}
		if (flag.name.size) {
			myAlignment += 4 + Utf8CharCount(flag.name);
		}
		alignment = max(alignment, myAlignment);
	}
	String flagExplanation;
	for (const Flag &flag : flags) {
		flagExplanation.ClearSoft();
		if (flag.letter) {
			AppendMultipleToString(flagExplanation, '-', EscapeString(Str(&flag.letter, 1), '"', false), ' ');
		}
		if (flag.name.size) {
			AppendMultipleToString(flagExplanation, "--", EscapeString(flag.name), ' ');
		}
		io::cout.PrintLn(flagExplanation, AlignText(alignment), flag.explanation);
	}
	io::cout.IndentLess();
}

// explanation is a human-readable explanation of how non-flag arguments are handled, used when generating a Usage function. This should represent the behavior of defaultHandler, if there is one.
// defaultHandler will receive any arguments that don't come from flags, and return false if there was an error.
// returns false if parsing failed (including if defaultHandler returned false)
inline bool ParseArguments(i32 argc, char *argv[], const Array<Flag> &flags=defs.flags, const Str &explanation=defs.explanation, const ArgHandler_t &defaultHandler=defs.defaultHandler) {
	defs.programName = argv[0];
	for (i32 i = 1; i < argc; i++) {
		Str arg = argv[i];
		Str nextArg = i+1 < argc ? Str(argv[i+1]) : Str(None);
		bool found = false;
		if (StartsWith(arg, "--")) {
			Str flagName = arg;
			RemoveFromBeginning(flagName, 2);
			for (const Flag &flag : flags) {
				if (flagName == flag.name) {
					if (flag.parseCallback(nextArg)) {
						i++; // Skip nextArg because we consumed it
					}
					found = true;
					break;
				}
			}
			if (!found) {
				io::cerr.PrintLn("Unknown flag \"", EscapeString(arg), "\"");
				PrintUsage(defs.programName, flags, explanation);
				return false;
			}
		} else if (StartsWith(arg, "-")) {
			Str chars = arg;
			RemoveFromBeginning(chars, 1);
			found = true;
			for (char flagLetter : chars) {
				bool foundChar = false;
				for (const Flag &flag : flags) {
					if (flag.letter == flagLetter) {
						if (flag.parseCallback(nextArg)) {
							i++; // Skip nextArg because we consumed it
							nextArg = i+1 < argc ? Str(argv[i+1]) : Str(None);
						}
						foundChar = true;
						break;
					}
				}
				if (!foundChar) {
					io::cerr.PrintLn("Unknown flag '", flagLetter, "'");
					PrintUsage(defs.programName, flags, explanation);
					return false;
				}
			}
		}
		if (!found) {
			if (defaultHandler) {
				if (!defaultHandler(arg)) return false;
			} else {
				io::cerr.PrintLn("Unhandled argument \"", EscapeString(arg), "\"");
				PrintUsage(defs.programName, flags, explanation);
				return false;
			}
		}
	}
	return true;
}

// TODO: TUI stuff

} // namespace AzCore::cli

#endif // AZCORE_CLI_HPP