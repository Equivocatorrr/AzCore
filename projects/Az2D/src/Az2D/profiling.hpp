/*
	File: profiling.hpp
	Author: Philip Haynes
	Important performance metrics, all found here.
*/

#ifndef AZ2D_PROFILING_HPP
#define AZ2D_PROFILING_HPP

#include "AzCore/Time.hpp"
#include "AzCore/Memory/StringArena.hpp"

namespace Az2D::Profiling {

AZCORE_CREATE_STRING_ARENA_HPP()

// Turns on profiling. Must be called at the beginning of the program.
void Enable();

// Outputs a log file containing all the times, or nothing if no profiling was done
void Report();

void Exception(AString scopeName, az::Nanoseconds time);

class Timer {
protected:
	AString scope;
	az::ClockTime start;
public:
	Timer(AString scopeName);
	void Start();
	void End();
};

class ScopedTimer : Timer {
public:
	ScopedTimer(AString scopeName);
	~ScopedTimer();
};

} // namespace Az2D::Profiling

#define AZ2D_PROFILING_SCOPED_TIMER(scopeName)\
	static Az2D::Profiling::AString _scopedTimerString(#scopeName);\
	static az::ClockTime _exceptionStart;\
	Az2D::Profiling::ScopedTimer _scopedTimer(_scopedTimerString);

#define AZ2D_PROFILING_EXCEPTION_START()\
	_exceptionStart = az::Clock::now();

#define AZ2D_PROFILING_EXCEPTION_END() \
	Az2D::Profiling::Exception(_scopedTimerString, az::Nanoseconds(az::Clock::now() - _exceptionStart));

#endif // AZ2D_PROFILING_HPP