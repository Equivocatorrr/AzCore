/*
	File: Log.cpp
	Author: Philip Haynes
*/

#include "Log.hpp"
#include "../Environment.hpp"

namespace AzCore {

namespace io {

#ifndef NDEBUG
LogLevel logLevel = LogLevel::DEBUG;
#else
LogLevel logLevel = LogLevel::RELEASE;
#endif

Log::~Log() {
	if (mFile) {
		fclose(mFile);
	}
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

Log& Log::UseLogFile(bool useFile, Str filename) {
	mFilename = filename;
	u32 lastSlash = 0;
	if (filename.size != 0) {
		for (i32 i = 0; i < filename.size; i++) {
			if (filename[i] == '\\' || filename[i] == '/') {
				lastSlash = i+1;
			}
		}
		Str prepend = filename.SubRange(lastSlash, filename.size-lastSlash);
		if (filename.size > 4 && filename.SubRange(filename.size-4, 4) == ".log") {
			prepend = prepend.SubRange(0, prepend.size-4);
		}
		mPrepend = Stringify("[", prepend, "] ");
		mPrepend.Resize(alignNonPowerOfTwo(mPrepend.size, mIndentString.size), ' ');
	}
	mLogFile = useFile;
	return *this;
}

Log& Log::Flush() {
	if (mLogConsole) {
		fflush(mConsoleFile);
	}
	if (mLogFile) {
		fflush(mFile);
	}
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

inline void StringIndent(String &str, i32 indent, String mIndentString) {
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
			StringIndent(consoleOut, indent, mIndentString);
		}
		if (mLogFile) {
			StringIndent(fileOut, indent, mIndentString);
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
					StringIndent(consoleOut, indent, mIndentString);
				}
			}
			if (mLogFile) {
				fileOut += range;
				if (i < out.size-1) {
					StringIndent(fileOut, indent, mIndentString);
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
		size_t written = fwrite(consoleOut.data, sizeof(char), consoleOut.size, mConsoleFile);
		if (written != (size_t)consoleOut.size) mLogConsole = false;
	}
}

template void Log::_Print<false>(SimpleRange<char>);
template void Log::_Print<true>(SimpleRange<char>);

Log& Log::PrintPlain(SimpleRange<char> out) {
	if (!mLogConsole && !mLogFile) return *this;
	_HandleFile();
	if (mFile) {
		size_t written = fwrite(out.str, sizeof(char), out.size, mFile);
		if (written != (size_t)out.size) mLogFile = false;
	}
	if (mLogConsole) {
		size_t written = fwrite(out.str, sizeof(char), out.size, mConsoleFile);
		if (written != (size_t)out.size) mLogConsole = false;
	}
	return *this;
}

Log& Log::PrintLnPlain(SimpleRange<char> out) {
	if (!mLogConsole && !mLogFile) return *this;
	_HandleFile();
	if (mFile) {
		size_t written = fwrite(out.str, sizeof(char), out.size, mFile);
		if (written != (size_t)out.size) mLogFile = false;
		fputc('\n', mFile);
	}
	if (mLogConsole) {
		size_t written = fwrite(out.str, sizeof(char), out.size, mConsoleFile);
		if (written != (size_t)out.size) mLogConsole = false;
		fputc('\n', mConsoleFile);
	}
	return *this;
}

Log& Log::Newline(i32 count) {
	if (!mLogConsole && !mLogFile) return *this;
	_HandleFile();
	if (mFile) {
		for (i32 i = 0; i < count; i++)
			fputc('\n', mFile);
	}
	if (mLogConsole) {
		for (i32 i = 0; i < count; i++)
			fputc('\n', mConsoleFile);
	}
	mStartOnNewline = true;
	return *this;
}

Log cout = Log(Str());
Log cerr = Log("stderr.log", true, true, stderr);

} // namespace io

} // namespace AzCore
