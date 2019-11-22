/*
    File: Time.hpp
    Author: Philip Haynes
*/

#ifndef AZCORE_TIME_HPP
#define AZCORE_TIME_HPP

#include <chrono>
#include "Memory/String.hpp"

namespace AzCore {

using Nanoseconds = std::chrono::nanoseconds;
using Microseconds = std::chrono::microseconds;
using Milliseconds = std::chrono::milliseconds;
using Seconds = std::chrono::seconds;
using Minutes = std::chrono::minutes;
using Hours = std::chrono::hours;

using Clock = std::chrono::steady_clock;
using ClockTime = std::chrono::steady_clock::time_point;

String FormatTime(Nanoseconds time);

} // namespace AzCore

#endif // AZCORE_TIME_HPP