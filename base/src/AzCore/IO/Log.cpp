/*
	File: Log.cpp
	Author: Philip Haynes
*/

#include "Log.hpp"

namespace AzCore {

namespace io {

Log::~Log() {
	if (file) fclose(file);
}

Log::Log(const Log &other) {
	logFile = other.logFile;
	logConsole = other.logConsole;
	indentString = other.indentString;
	indent = other.indent;
	prepend = other.prepend;
	filename = other.filename + "_d";
}

Log& Log::operator=(const Log &other) {
	if (file) fclose(file);
	file = nullptr;
	openAttempt = false;
	logFile = other.logFile;
	logConsole = other.logConsole;
	indentString = other.indentString;
	indent = other.indent;
	prepend = other.prepend;
	filename = other.filename + "_d";
	return *this;
}

inline void Log::_HandleFile() {
	if (!logFile) return;
	if (openAttempt) return;

	file = fopen(filename.data, "w");
	if (!file) {
		logFile = false;
	}

	openAttempt = true;
}

inline void Indent(String &str, i32 indent, String indentString) {
	if (str.size && indent) {
		for (i32 i = 0; i < indent; i++) {
			str.Append(indentString);
		}
	}
}

template<bool newline>
void Log::_Print(SimpleRange<char> out) {
#ifndef NDEBUG
	if (out == "\n") {
		out = "\nPlease use Log::Newline() instead of Log::Print(\"\\n\")\n";
	}
#endif
	if (!logConsole && !logFile) return;
	_HandleFile();
	static String consoleOut;
	static String fileOut;
	if ((!logConsole || prepend.size == 0) && indent == 0) {
		if constexpr (newline) {
			PrintLnPlain(out);
		} else {
			PrintPlain(out);
		}
		return;
	}
	// Soft reset since we don't want to reallocate all the time
	consoleOut.size = 0;
	fileOut.size = 0;
	if (startOnNewline && out.size && out[0] != '\n' && out[0] != '\r') {
		if (logConsole) {
			consoleOut = prepend;
			Indent(consoleOut, indent, indentString);
		}
		if (logFile) {
			Indent(fileOut, indent, indentString);
		}
	}
	i32 i = 0;
	i32 last = 0;
	for (; i < out.size; i++) {
		char c = out[i];
		if (c == '\n' || c == '\r') {
			SimpleRange<char> range = out.SubRange(last, i-last+1);
			if (logConsole) {
				consoleOut += range;
				if (i < out.size-1) {
					consoleOut += prepend;
					Indent(consoleOut, indent, indentString);
				}
			}
			if (logFile) {
				fileOut += range;
				if (i < out.size-1) {
					Indent(fileOut, indent, indentString);
				}
			}
			last = i+1;
		}
	}
	if (i != last) {
		SimpleRange<char> range = out.SubRange(last, i-last);
		if (logConsole) {
			consoleOut += range;
			if constexpr (newline) {
				consoleOut += '\n';
			}
		}
		if (logFile) {
			fileOut += range;
			if constexpr (newline) {
				fileOut += '\n';
			}
		}
		if constexpr (newline) {
			startOnNewline = true;
		} else {
			startOnNewline = false;
		}
	} else {
		if (logConsole) consoleOut += '\n';
		if (logFile) fileOut += '\n';
		startOnNewline = true;
	}
	if (file) {
		size_t written = fwrite(fileOut.data, sizeof(char), fileOut.size, file);
		if (written != (size_t)fileOut.size) logFile = false;
	}
	if (logConsole) {
		size_t written = fwrite(consoleOut.data, sizeof(char), consoleOut.size, stdout);
		if (written != (size_t)consoleOut.size) logConsole = false;
	}
}

template void Log::_Print<false>(SimpleRange<char>);
template void Log::_Print<true>(SimpleRange<char>);

void Log::PrintPlain(SimpleRange<char> out) {
	if (!logConsole && !logFile) return;
	_HandleFile();
	if (file) {
		size_t written = fwrite(out.str, sizeof(char), out.size, file);
		if (written != (size_t)out.size) logFile = false;
	}
	if (logConsole) {
		size_t written = fwrite(out.str, sizeof(char), out.size, stdout);
		if (written != (size_t)out.size) logConsole = false;
	}
}

void Log::PrintLnPlain(SimpleRange<char> out) {
	if (!logConsole && !logFile) return;
	_HandleFile();
	if (file) {
		size_t written = fwrite(out.str, sizeof(char), out.size, file);
		if (written != (size_t)out.size) logFile = false;
		fputc('\n', file);
	}
	if (logConsole) {
		size_t written = fwrite(out.str, sizeof(char), out.size, stdout);
		if (written != (size_t)out.size) logConsole = false;
		fputc('\n', stdout);
	}
}

void Log::Newline(i32 count) {
	if (!logConsole && !logFile) return;
	_HandleFile();
	if (file) {
		for (i32 i = 0; i < count; i++)
			fputc('\n', file);
	}
	if (logConsole) {
		for (i32 i = 0; i < count; i++)
			fputc('\n', stdout);
	}
	startOnNewline = true;
}

} // namespace io

} // namespace AzCore
