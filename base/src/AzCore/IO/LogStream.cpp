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
    u32 lastSlash = 0;
    for (i32 i = 0; i < filename.size; i++) {
        if (filename[i] == '\\' || filename[i] == '/') {
            lastSlash = i+1;
        }
    }
    prepend = "[";
    prepend += filename.data+lastSlash;
    prepend += "] ";
    for (u32 i = prepend.size; i <= 16; i++) {
        prepend += " ";
    }
}

LogStream::LogStream(const LogStream &other) :
fstream(), openAttempt(false), logFile(true), logConsole(other.logConsole),
flushed(true), prepend(other.prepend), filename(other.filename+"_d") {}

LogStream& LogStream::operator=(const LogStream &other) {
    if (fstream.is_open()) {
        fstream.close();
    }
    openAttempt = false;
    logFile = true;
    logConsole = other.logConsole;
    flushed = true;
    prepend = other.prepend;
    filename = other.filename+"_d";
    return *this;
}

LogStream& LogStream::operator<<(const char* string) {
    const char *sixteenSpaces = "                ";
    HandleFileOpening();
    if (logConsole) {
        String actualOutput;
        if (prepend.size+indent != 0) {
            if (flushed) {
                if (prepend.size) {
                    std::cout.write(prepend.data, prepend.size);
                }
                for (i32 i = 0; i < indent; i++) {
                    std::cout.write(sixteenSpaces, spacesPerIndent);
                }
                flushed = false;
            }
            for (u32 i = 0; string[i] != '\0'; i++) {
                if (string[i] == '\n') {
                    actualOutput += "\n" + prepend;
                    for (i32 ii = 0; ii < indent; ii++) {
                        actualOutput += sixteenSpaces + 16 - spacesPerIndent;
                    }
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
