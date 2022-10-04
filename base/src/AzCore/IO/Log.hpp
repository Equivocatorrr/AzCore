/*
	File: Log.hpp
	Author: Philip Haynes
	Replacing LogStream with more classic-style print functions.
*/

#ifndef AZCORE_LOG_HPP
#define AZCORE_LOG_HPP

#include "../memory.hpp"
#include "../Thread.hpp"

namespace AzCore {

namespace io {

/*  Use this to write any and all console output. */
class Log {
	FILE *mFile = nullptr;
	bool mOpenAttempt = false;
	bool mLogFile = false;
	bool mLogConsole = true;
	bool mStartOnNewline = true;
	String mIndentString = "    ";
	Mutex mMutex;
	String mPrepend;
	String mFilename;
	inline void _HandleFile();
public:
	i32 indent = 0;
	Log() = default;
	inline Log(String filename, bool useConsole=true, bool useFile=false) : Log() {
		mFilename = filename;
		u32 lastSlash = 0;
		for (i32 i = 0; i < mFilename.size; i++) {
			if (mFilename[i] == '\\' || mFilename[i] == '/') {
				lastSlash = i+1;
			}
		}
		mPrepend = Stringify("[", mFilename.GetRange(lastSlash, mFilename.size-lastSlash), "] ");
		mPrepend.Resize(align(mPrepend.size, mIndentString.size), ' ');
		mLogConsole = useConsole;
		mLogFile = useFile;
	}
	~Log();

	Log(const Log &other);
	Log& operator=(const Log &other);
	Log(Log &&other) = default;
	Log& operator=(Log &&other) = default;

	void UseLogFile(bool useFile=true) {
		mLogFile = useFile;
	}

	void NoLogFile() {
		static bool once = false;
		if (!once) {
			Print("NoLogFile() is deprecated, and Log by default doesn't use a file. Switch to UseLogFile(bool)");
			once = true;
		}
		mLogFile = false;
	}

	template<bool newline>
	void _Print(SimpleRange<char> out);

	inline void Print(String out) {
		_Print<false>(out);
	}
	inline void Print(const char *out) {
		_Print<false>(out);
	}
	inline void Print(char *out) {
		_Print<false>(out);
	}
	inline void Print(SimpleRange<char> out) {
		_Print<false>(out);
	}

	inline void PrintLn(String out) {
		_Print<true>(out);
	}
	inline void PrintLn(const char *out) {
		_Print<true>(out);
	}
	inline void PrintLn(char *out) {
		_Print<true>(out);
	}
	inline void PrintLn(SimpleRange<char> out) {
		_Print<true>(out);
	}

	template <typename... Args>
	inline void Print(Args... args) {
		Print(Stringify(args...));
	}
	template <typename... Args>
	inline void PrintLn(Args... args) {
		PrintLn(Stringify(args...));
	}
	// Print without indenting or prepending on newlines
	void PrintPlain(SimpleRange<char> out);
	void PrintLnPlain(SimpleRange<char> out);

	void Newline(i32 count = 1);

	inline void IndentMore() {
		indent++;
	}
	inline void IndentLess() {
		indent--;
	}
	inline void Lock() {
		mMutex.Lock();
	}
	inline void Unlock() {
		mMutex.Unlock();
	}
	inline void IndentString(String value) {
		if (value.size == 0) value = " ";
		mIndentString = value;
		mPrepend.Resize(mFilename.size + 3);
		mPrepend.Resize(align(mPrepend.size, mIndentString.size), ' ');
	}
};

} // namespace io

} // namespace AzCore

#endif // AZCORE_LOG_HPP
