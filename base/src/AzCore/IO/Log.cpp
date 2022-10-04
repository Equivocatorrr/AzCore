/*
	File: Log.cpp
	Author: Philip Haynes
*/

#include "Log.hpp"

namespace AzCore {

namespace io {

Log::~Log() {
	if (mFile) fclose(mFile);
}

Log::Log(const Log &other) {
	mLogFile = other.mLogFile;
	mLogConsole = other.mLogConsole;
	mIndentString = other.mIndentString;
	indent = other.indent;
	mPrepend = other.mPrepend;
	mFilename = other.mFilename + "_d";
}

Log& Log::operator=(const Log &other) {
	if (mFile) fclose(mFile);
	mFile = nullptr;
	mOpenAttempt = false;
	mLogFile = other.mLogFile;
	mLogConsole = other.mLogConsole;
	mIndentString = other.mIndentString;
	indent = other.indent;
	mPrepend = other.mPrepend;
	mFilename = other.mFilename + "_d";
	return *this;
}

inline void Log::_HandleFile() {
	if (!mLogFile) return;
	if (mOpenAttempt) return;

	mFile = fopen(mFilename.data, "w");
	if (!mFile) {
		mLogFile = false;
	}

	mOpenAttempt = true;
}

inline void Indent(String &str, i32 indent, String mIndentString) {
	if (str.size && indent) {
		for (i32 i = 0; i < indent; i++) {
			str.Append(mIndentString);
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
	if (!mLogConsole && !mLogFile) return;
	_HandleFile();
	static String consoleOut;
	static String fileOut;
	if ((!mLogConsole || mPrepend.size == 0) && indent == 0) {
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
	if (mStartOnNewline && out.size && out[0] != '\n' && out[0] != '\r') {
		if (mLogConsole) {
			consoleOut = mPrepend;
			Indent(consoleOut, indent, mIndentString);
		}
		if (mLogFile) {
			Indent(fileOut, indent, mIndentString);
		}
	}
	i32 i = 0;
	i32 last = 0;
	for (; i < out.size; i++) {
		char c = out[i];
		if (c == '\n' || c == '\r') {
			SimpleRange<char> range = out.SubRange(last, i-last+1);
			if (mLogConsole) {
				consoleOut += range;
				if (i < out.size-1) {
					consoleOut += mPrepend;
					Indent(consoleOut, indent, mIndentString);
				}
			}
			if (mLogFile) {
				fileOut += range;
				if (i < out.size-1) {
					Indent(fileOut, indent, mIndentString);
				}
			}
			last = i+1;
		}
	}
	if (i != last) {
		SimpleRange<char> range = out.SubRange(last, i-last);
		if (mLogConsole) {
			consoleOut += range;
			if constexpr (newline) {
				consoleOut += '\n';
			}
		}
		if (mLogFile) {
			fileOut += range;
			if constexpr (newline) {
				fileOut += '\n';
			}
		}
		if constexpr (newline) {
			mStartOnNewline = true;
		} else {
			mStartOnNewline = false;
		}
	} else {
		if (mLogConsole) consoleOut += '\n';
		if (mLogFile) fileOut += '\n';
		mStartOnNewline = true;
	}
	if (mFile) {
		size_t written = fwrite(fileOut.data, sizeof(char), fileOut.size, mFile);
		if (written != (size_t)fileOut.size) mLogFile = false;
	}
	if (mLogConsole) {
		size_t written = fwrite(consoleOut.data, sizeof(char), consoleOut.size, stdout);
		if (written != (size_t)consoleOut.size) mLogConsole = false;
	}
}

template void Log::_Print<false>(SimpleRange<char>);
template void Log::_Print<true>(SimpleRange<char>);

void Log::PrintPlain(SimpleRange<char> out) {
	if (!mLogConsole && !mLogFile) return;
	_HandleFile();
	if (mFile) {
		size_t written = fwrite(out.str, sizeof(char), out.size, mFile);
		if (written != (size_t)out.size) mLogFile = false;
	}
	if (mLogConsole) {
		size_t written = fwrite(out.str, sizeof(char), out.size, stdout);
		if (written != (size_t)out.size) mLogConsole = false;
	}
}

void Log::PrintLnPlain(SimpleRange<char> out) {
	if (!mLogConsole && !mLogFile) return;
	_HandleFile();
	if (mFile) {
		size_t written = fwrite(out.str, sizeof(char), out.size, mFile);
		if (written != (size_t)out.size) mLogFile = false;
		fputc('\n', mFile);
	}
	if (mLogConsole) {
		size_t written = fwrite(out.str, sizeof(char), out.size, stdout);
		if (written != (size_t)out.size) mLogConsole = false;
		fputc('\n', stdout);
	}
}

void Log::Newline(i32 count) {
	if (!mLogConsole && !mLogFile) return;
	_HandleFile();
	if (mFile) {
		for (i32 i = 0; i < count; i++)
			fputc('\n', mFile);
	}
	if (mLogConsole) {
		for (i32 i = 0; i < count; i++)
			fputc('\n', stdout);
	}
	mStartOnNewline = true;
}

} // namespace io

} // namespace AzCore
