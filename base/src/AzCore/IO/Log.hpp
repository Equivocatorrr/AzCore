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
    bool logFile = true;
    bool logConsole = true;
    bool startOnNewline = true;
    i32 spacesPerIndent = 4;
    i32 indent = 0;
    Mutex mutex;
    String prepend;
    String filename;
    inline void _HandleFile();
public:
    Log() = default;
    inline Log(String logFilename, bool console=true) : Log() {
        filename = logFilename;
        u32 lastSlash = 0;
        for (i32 i = 0; i < filename.size; i++) {
            if (filename[i] == '\\' || filename[i] == '/') {
                lastSlash = i+1;
            }
        }
        prepend = "[" + filename.GetRange(lastSlash, filename.size-lastSlash) + "] ";
        prepend.Resize(align(prepend.size, spacesPerIndent), ' ');
        logConsole = console;
    }
    ~Log();

    Log(const Log &other);
    Log& operator=(const Log &other);
    Log(Log &&other) = default;
    Log& operator=(Log &&other) = default;

    void NoLogFile() {
        logFile = false;
    }

    template <typename T>
    inline void Print(T out) {
        Print(ToString(out));
    }
    template <typename T, typename... Args>
    inline void Print(T first, Args... args) {
        Print(first);
        Print<Args...>(args...);
    }
    template <typename T>
    inline void PrintLn(T out) {
        PrintLn(ToString(out));
    }
    template <typename T, typename... Args>
    inline void PrintLn(T first, Args... args) {
        Print(first);
        PrintLn<Args...>(args...);
    }
    // Print without indenting or prepending on newlines
    void PrintPlain(Range<char> out);
    void PrintLnPlain(Range<char> out);

    template<bool newline>
    void _Print(Range<char> out);

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
    inline void SpacesPerIndent(i32 value) {
        if (value < 1) value = 1;
        spacesPerIndent = value;
        prepend.Resize(filename.size + 3);
        prepend.Resize(align(prepend.size, spacesPerIndent), ' ');
    }
};

template<>
inline void Log::Print(String out) {
    _Print<false>(out.GetRange(0, out.size));
}
template<>
inline void Log::Print(const char *out) {
    _Print<false>(Range<char>((char*)out, StringLength(out)));
}
template<>
inline void Log::Print(char *out) {
    _Print<false>(Range<char>(out, StringLength(out)));
}

template<>
inline void Log::PrintLn(String out) {
    _Print<true>(out.GetRange(0, out.size));
}
template<>
inline void Log::PrintLn(const char *out) {
    _Print<true>(Range<char>((char*)out, StringLength(out)));
}
template<>
inline void Log::PrintLn(char *out) {
    _Print<true>(Range<char>(out, StringLength(out)));
}

} // namespace io

} // namespace AzCore

#endif // AZCORE_LOG_HPP
