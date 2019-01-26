/*
    File: logStream.hpp
    Author: Philip Haynes
    Logging utilities.
*/
#ifndef LOG_STREAM_HPP
#define LOG_STREAM_HPP

#include "common.hpp"

#include <iostream>
#include <fstream>

namespace io {

    /*  class: logStream
    Author: Philip Haynes
    Use this class to write any and all debugging/status text.
    Use it the same way you would std::cout.
    Ex: io::cout << "Say it ain't so!!" << std::endl;
    Entries in this class will be printed to terminal and a log file.   */
    class logStream {
        std::ofstream fstream;
        bool log; // Whether we're using a log file
        Mutex mutex;
    public:
        logStream();
        logStream(String logFilename);
        template<typename T> logStream& operator<<(const T& something) {
            std::cout << something;
            if (log)
                fstream << something;
            return *this;
        }
        typedef std::ostream& (*stream_function)(std::ostream&);
        logStream& operator<<(stream_function func);
        void MutexLock();
        void MutexUnlock();
    };

}

#endif
