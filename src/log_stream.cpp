/*
    File: log_stream.cpp
    Author: Philip Haynes
*/
#include "log_stream.hpp"

namespace io {
    
    logStream::logStream() : fstream("log.txt") , log(true) {
        if (!fstream.is_open()) {
            std::cout << "Failed to open log.txt for writing" << std::endl;
            log = false;
        }
    }

    logStream::logStream(String logFilename) : fstream(logFilename) , log(true) {
        if (!fstream.is_open()) {
            std::cout << "Failed to open " << logFilename << " for writing" << std::endl;
            log = false;
        }
    }

    logStream& logStream::operator<<(stream_function func) {
        func(std::cout);
        if (log)
        func(fstream);
        return *this;
    }

    void logStream::MutexLock() {
        mutex.lock();
    }

    void logStream::MutexUnlock() {
        mutex.unlock();
    }

}
