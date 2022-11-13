/*
	File: Log.hpp
	Author: Philip Haynes
	Replacing LogStream with more classic-style print functions.
*/

#ifndef AZCORE_LOG_HPP
#define AZCORE_LOG_HPP

#include "../Memory/String.hpp"
#include "../Memory/Range.hpp"
#include "../Thread.hpp"

namespace AzCore {

namespace io {

/*  Use this to write any and all console output. */
class Log {
	FILE *mFile = nullptr;
	FILE *mConsoleFile = stdout;
	bool mOpenAttempt = false;
	bool mLogFile = false;
	bool mLogConsole = true;
	bool mStartOnNewline = true;
	String mIndentString = "    ";
	Mutex mMutex;
	String mPrepend;
	String mFilename;

	inline void _HandleFile();

	template<bool newline>
	void _Print(SimpleRange<char> out);
public:
	i32 indent = 0;

	Log() = default;
	inline Log(String filename, bool useConsole=true, bool useFile=false, FILE *consoleFile=stdout) : Log() {
		mConsoleFile = consoleFile;
		UseLogFile(useFile, filename);
		mLogConsole = useConsole;
	}
	~Log();

	Log(const Log &other);
	Log& operator=(const Log &other);
	Log(Log &&other) = default;
	Log& operator=(Log &&other) = default;

	Log& UseLogFile(bool useFile=true, SimpleRange<char> filename="") {
		mFilename = filename;
		u32 lastSlash = 0;
		if (mFilename.size != 0) {
			for (i32 i = 0; i < mFilename.size; i++) {
				if (mFilename[i] == '\\' || mFilename[i] == '/') {
					lastSlash = i+1;
				}
			}
			mPrepend = Stringify("[", mFilename.GetRange(lastSlash, mFilename.size-lastSlash), "] ");
			mPrepend.Resize(align(mPrepend.size, mIndentString.size), ' ');
		}
		mLogFile = useFile;
		return *this;
	}

	[[deprecated("NoLogFile() is deprecated, and Log by default doesn't use a file. Switch to UseLogFile(bool)")]]
	void NoLogFile() {
		mLogFile = false;
	}

	// Forces all buffered outputs to be flushed.
	Log& Flush();

	inline Log& Print(String out) {
		_Print<false>(out);
		return *this;
	}
	inline Log& Print(const char *out) {
		_Print<false>(out);
		return *this;
	}
	inline Log& Print(char *out) {
		_Print<false>(out);
		return *this;
	}
	inline Log& Print(SimpleRange<char> out) {
		_Print<false>(out);
		return *this;
	}

	inline Log& PrintLn(String out) {
		_Print<true>(out);
		return *this;
	}
	inline Log& PrintLn(const char *out) {
		_Print<true>(out);
		return *this;
	}
	inline Log& PrintLn(char *out) {
		_Print<true>(out);
		return *this;
	}
	inline Log& PrintLn(SimpleRange<char> out) {
		_Print<true>(out);
		return *this;
	}

	template <typename... Args>
	inline Log& Print(Args... args) {
		Print(Stringify(args...));
		return *this;
	}
	template <typename... Args>
	inline Log& PrintLn(Args... args) {
		PrintLn(Stringify(args...));
		return *this;
	}
	// Print without indenting or prepending on newlines
	Log& PrintPlain(SimpleRange<char> out);
	// Print without indenting or prepending on newlines
	Log& PrintLnPlain(SimpleRange<char> out);

	// Outputs count newlines (default 1).
	Log& Newline(i32 count = 1);

	// Increase indent by one
	inline Log& IndentMore() {
		indent++;
		return *this;
	}
	// Decrease indent by one
	inline Log& IndentLess() {
		indent--;
		return *this;
	}
	// Locks this Log's mutex, allowing thread-safe output.
	// NOTE: All threads must call this to be thread-safe.
	inline Log& Lock() {
		mMutex.Lock();
		return *this;
	}
	// Unlocks the mutex.
	inline Log& Unlock() {
		mMutex.Unlock();
		return *this;
	}
	// Changes the string we use for indenting. Default is 4 spaces.
	inline Log& IndentString(SimpleRange<char> value) {
		if (value.size == 0) value = " ";
		mIndentString = value;
		mPrepend.Resize(mFilename.size + 3);
		mPrepend.Resize(align(mPrepend.size, mIndentString.size), ' ');
		return *this;
	}
};

// Simple output to stdout
extern Log cout;
// Simple output to stderr
extern Log cerr;

} // namespace io

} // namespace AzCore

#endif // AZCORE_LOG_HPP
