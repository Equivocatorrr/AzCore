/*
    File: LogStream.hpp
    Author: Philip Haynes
    Logging utilities.
*/
#ifndef AZCORE_LOGSTREAM_HPP
#define AZCORE_LOGSTREAM_HPP

#include "../memory.hpp"
#include "../Thread.hpp"

#include <iostream>
#include <iomanip>
#include <fstream>

namespace AzCore {

namespace io {

/*  class: LogStream
Author: Philip Haynes
Use this class to write any and all debugging/status text.
Use it the same way you would std::cout.
Ex: io::cout << "Say it ain't so!!" << std::endl;
Entries in this class will be printed to terminal and a log file.   */
class LogStream {
    std::ofstream fstream;
    bool openAttempt;
    bool logFile;
    bool logConsole;
    bool flushed;
    Mutex mutex;
    String prepend;
    String filename;

    inline void HandleFileOpening() {
        if (openAttempt) return;

        fstream.open(filename.data);
        if (!fstream.is_open()) {
            logFile = false;
        }

        openAttempt = true;
    }
public:
    LogStream();
    LogStream(String logFilename, bool console=true);
    LogStream(const LogStream &other);
    LogStream& operator=(const LogStream& other);
    LogStream(LogStream &&other) = default;
    LogStream& operator=(LogStream&& other) = default;
    template<typename T> LogStream& operator<<(const T& something) {
        HandleFileOpening();
        if (logConsole) {
            if (flushed && prepend.size != 0) {
                std::cout.write(prepend.data, prepend.size);
                flushed = false;
            }
            std::cout << something;
        }
        if (logFile)
            fstream << something;
        return *this;
    }
    LogStream& operator<<(const char*);
    LogStream& operator<<(const String&);
    typedef std::ostream& (*stream_function)(std::ostream&);
    LogStream& operator<<(stream_function func);
    void MutexLock();
    void MutexUnlock();
};

} // namespace io

} // namespace AzCore

#endif // AZCORE_LOGSTREAM_HPP
