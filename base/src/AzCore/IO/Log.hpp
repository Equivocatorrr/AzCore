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
	FILE *file = nullptr;
	bool openAttempt = false;
	bool logFile = false;
	bool logConsole = true;
	bool startOnNewline = true;
	String indentString = "    ";
	Mutex mutex;
	String prepend;
	String filename;
	inline void _HandleFile();
public:
	i32 indent = 0;
	Log() = default;
	inline Log(String logFilename, bool console=true) : Log() {
		filename = logFilename;
		u32 lastSlash = 0;
		for (i32 i = 0; i < filename.size; i++) {
			if (filename[i] == '\\' || filename[i] == '/') {
				lastSlash = i+1;
			}
		}
		prepend = Stringify("[", filename.GetRange(lastSlash, filename.size-lastSlash), "] ");
		prepend.Resize(align(prepend.size, indentString.size), ' ');
		logConsole = console;
		logFile = true;
	}
	~Log();

	Log(const Log &other);
	Log& operator=(const Log &other);
	Log(Log &&other) = default;
	Log& operator=(Log &&other) = default;

	void NoLogFile() {
		logFile = false;
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
		mutex.Lock();
	}
	inline void Unlock() {
		mutex.Unlock();
	}
	inline void IndentString(String value) {
		if (value.size == 0) value = " ";
		indentString = value;
		prepend.Resize(filename.size + 3);
		prepend.Resize(align(prepend.size, indentString.size), ' ');
	}
};

} // namespace io

} // namespace AzCore

#endif // AZCORE_LOG_HPP
