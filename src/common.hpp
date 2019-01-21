/*
    File: common.hpp
    Author: Philip Haynes
    Description: Meta-file to include math and memory aliases.
*/
#ifndef COMMON_HPP
#define COMMON_HPP

#include "math.hpp"
#include "memory.hpp"

#include <mutex>
// #ifdef __MINGW32__
// #include <mingw/mutex.h>
// #endif

using Mutex = std::mutex;

#endif
