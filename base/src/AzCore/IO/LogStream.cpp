/*
    File: LogStream.cpp
    Author: Philip Haynes
*/
#include "LogStream.hpp"

namespace AzCore {

namespace io {

LogStream::LogStream() :
fstream(), openAttempt(false), logFile(true), logConsole(true),
flushed(true), prepend(""), filename("log.log") {}

LogStream::LogStream(String logFilename, bool console) :
fstream(), openAttempt(false), logFile(true),
logConsole(console), flushed(true), filename(logFilename) {
    String logFilenameNoDirectories;
    u32 lastSlash = 0;
    for (i32 i = 0; i < logFilename.size; i++) {
        if (logFilename[i] == '\\' || logFilename[i] == '/') {
            lastSlash = i+1;
        }
    }
    prepend = "[";
    prepend += logFilename.data+lastSlash;
    prepend += "] ";
    for (u32 i = prepend.size; i <= 16; i++) {
        prepend += " ";
    }
}

LogStream& LogStream::operator<<(const char* string) {
    HandleFileOpening();
    if (logConsole) {
        String actualOutput;
        if (prepend.size != 0) {
            if (flushed) {
                std::cout.write(prepend.data, prepend.size);
                flushed = false;
            }
            for (u32 i = 0; string[i] != '\0'; i++) {
                if (string[i] == '\n') {
                    actualOutput += "\n" + prepend;
                } else {
                    actualOutput += string[i];
                }
            }
            std::cout.write(actualOutput.data, actualOutput.size);
        } else {
            i32 size = 0;
            for (; string[size] != 0; size++) {}
            std::cout.write(string, size);
        }
    }
    if (logFile) {
        i32 size = 0;
        for (; string[size] != 0; size++) {}
        fstream.write(string, size);
    }
    return *this;
}

LogStream& LogStream::operator<<(const String& string) {
    return *this << (const char*)string.data;
}

LogStream& LogStream::operator<<(stream_function func) {
    HandleFileOpening();
    if (logConsole) {
        if (func == &std::endl<char, std::char_traits<char>>) {
            flushed = true;
        }
        func(std::cout);
    }
    if (logFile) {
        func(fstream);
    }
    return *this;
}

void LogStream::MutexLock() {
    mutex.Lock();
}

void LogStream::MutexUnlock() {
    mutex.Unlock();
}

} // namespace io

} // namespace AzCore
