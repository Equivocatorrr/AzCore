/*
	File: Log.cpp
	Author: Philip Haynes
*/

#include "Log.hpp"

namespace AzCore {

namespace io {

static Mutex consoleMutex;

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
		ScopedLock lock(consoleMutex);
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
	if (indent) {
		for (i32 i = 0; i < indent; i++) {
			str.Append(mIndentString);
		}
	}
}

template<bool newline>
void Log::_Print(Str out) {
	if (out.size == 1 && out.data[0] == '\n') {
		Newline(1);
		return;
	}
	ScopedLock lock(mMutex);
	if (!mLogConsole && !mLogFile) return;
	_HandleFile();
	if ((!mLogConsole || mPrepend.size == 0) && indent == 0) {
		lock.Release();
		if constexpr (newline) {
			PrintLnPlain(out);
		} else {
			PrintPlain(out);
		}
		return;
	}
	_consoleOut.ClearSoft();
	_fileOut.ClearSoft();
	if (mStartOnNewline && out.size && out[0] != '\n' && out[0] != '\r') {
		if (mLogConsole) {
			_consoleOut = mPrepend;
			StringIndent(_consoleOut, indent, mIndentString);
		}
		if (mLogFile) {
			StringIndent(_fileOut, indent, mIndentString);
		}
	}
	i32 i = 0;
	i32 last = 0;
	for (; i < out.size; i++) {
		char c = out[i];
		if (c == '\n' || c == '\r') {
			Str range = out.SubRange(last, i-last+1);
			if (mLogConsole) {
				_consoleOut += range;
				if (i < out.size-1) {
					_consoleOut += mPrepend;
					StringIndent(_consoleOut, indent, mIndentString);
				}
			}
			if (mLogFile) {
				_fileOut += range;
				if (i < out.size-1) {
					StringIndent(_fileOut, indent, mIndentString);
				}
			}
			last = i+1;
		}
	}
	if (i != last) {
		Str range = out.SubRange(last, i-last);
		if (mLogConsole) {
			_consoleOut += range;
			if constexpr (newline) {
				_consoleOut += '\n';
			}
		}
		if (mLogFile) {
			_fileOut += range;
			if constexpr (newline) {
				_fileOut += '\n';
			}
		}
		if constexpr (newline) {
			mStartOnNewline = true;
		} else {
			mStartOnNewline = false;
		}
	} else {
		if (mLogConsole) _consoleOut += '\n';
		if (mLogFile) _fileOut += '\n';
		mStartOnNewline = true;
	}
	if (mFile) {
		size_t written = fwrite(_fileOut.data, sizeof(char), _fileOut.size, mFile);
		if (written != (size_t)_fileOut.size) mLogFile = false;
	}
	if (mLogConsole) {
		ScopedLock lock(consoleMutex);
		size_t written = fwrite(_consoleOut.data, sizeof(char), _consoleOut.size, mConsoleFile);
		if (written != (size_t)_consoleOut.size) mLogConsole = false;
	}
}

template void Log::_Print<false>(Str);
template void Log::_Print<true>(Str);

Log& Log::PrintPlain(Str out) {
	ScopedLock lock(mMutex);
	if (!mLogConsole && !mLogFile) return *this;
	_HandleFile();
	if (mFile) {
		size_t written = fwrite(out.data, sizeof(char), out.size, mFile);
		if (written != (size_t)out.size) mLogFile = false;
	}
	if (mLogConsole) {
		ScopedLock lock(consoleMutex);
		size_t written = fwrite(out.data, sizeof(char), out.size, mConsoleFile);
		if (written != (size_t)out.size) mLogConsole = false;
	}
	return *this;
}

Log& Log::PrintLnPlain(Str out) {
	ScopedLock lock(mMutex);
	if (!mLogConsole && !mLogFile) return *this;
	_HandleFile();
	if (mFile) {
		size_t written = fwrite(out.data, sizeof(char), out.size, mFile);
		if (written != (size_t)out.size) mLogFile = false;
		fputc('\n', mFile);
	}
	if (mLogConsole) {
		ScopedLock lock(consoleMutex);
		size_t written = fwrite(out.data, sizeof(char), out.size, mConsoleFile);
		if (written != (size_t)out.size) mLogConsole = false;
		fputc('\n', mConsoleFile);
	}
	return *this;
}

Log& Log::Newline(i32 count) {
	ScopedLock lock(mMutex);
	if (!mLogConsole && !mLogFile) return *this;
	_HandleFile();
	if (mFile) {
		for (i32 i = 0; i < count; i++)
			fputc('\n', mFile);
	}
	if (mLogConsole) {
		ScopedLock lock(consoleMutex);
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
