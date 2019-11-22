/*
    File: LogStream.hpp
    Author: Philip Haynes
    Logging utilities.
*/
#ifndef AZCORE_LOGSTREAM_HPP
#define AZCORE_LOGSTREAM_HPP

#include "../memory.hpp"

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
    bool logFile;
    bool logConsole;
    bool flushed;
    Mutex mutex;
    String prepend;
public:
    LogStream();
    LogStream(String logFilename, bool console=true);
    template<typename T> LogStream& operator<<(const T& something) {
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
