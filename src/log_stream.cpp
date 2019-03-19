/*
    File: log_stream.cpp
    Author: Philip Haynes
*/
#include "log_stream.hpp"

namespace io {

    logStream::logStream() : fstream("log.txt") , log(true), flushed(true) , prepend("") {
        if (!fstream.is_open()) {
            std::cout << "Failed to open log.txt for writing" << std::endl;
            log = false;
        }
    }

    logStream::logStream(String logFilename) : fstream(logFilename) , log(true) , flushed(true) {
        String logFilenameNoDirectories;
        u32 lastSlash = 0;
        for (u32 i = 0; i < logFilename.size(); i++) {
            if (logFilename[i] == '\\' || logFilename[i] == '/') {
                lastSlash = i+1;
            }
        }
        prepend = "[";
        prepend += logFilename.c_str()+lastSlash;
        prepend += "] ";
        for (u32 i = prepend.size(); i <= 16; i++) {
            prepend += " ";
        }
        if (!fstream.is_open()) {
            std::cout << "Failed to open " << logFilename << " for writing" << std::endl;
            log = false;
        }
    }

    logStream& logStream::operator<<(const char* string) {
        String actualOutput = "";
        if (prepend.size() != 0) {
            if (flushed) {
                std::cout << prepend;
                flushed = false;
            }
            for (u32 i = 0; string[i] != '\0'; i++) {
                if (string[i] == '\n') {
                    actualOutput += "\n" + prepend;
                } else {
                    actualOutput += string[i];
                }
            }
            std::cout << actualOutput;
        } else {
            std::cout << string;
        }
        if (log) {
            fstream << string;
        }
        return *this;
    }

    logStream& logStream::operator<<(const String& string) {
        return *this << string.c_str();
    }

    logStream& logStream::operator<<(stream_function func) {
        if (func == &std::endl<char, std::char_traits<char>>) {
            flushed = true;
        }
        func(std::cout);
        if (log) {
            func(fstream);
        }
        return *this;
    }

    void logStream::MutexLock() {
        mutex.lock();
    }

    void logStream::MutexUnlock() {
        mutex.unlock();
    }

}
