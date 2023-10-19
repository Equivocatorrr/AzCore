/*
	File: Profiling.hpp
	Author: Philip Haynes
	Important performance metrics, all found here.
*/

#ifndef AZCORE_PROFILING_HPP
#define AZCORE_PROFILING_HPP

#include "Time.hpp"
#include "Memory/StringArena.hpp"

namespace AzCore::Profiling {

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
	inline ScopedTimer(AString scopeName) : Timer(scopeName) { Start(); }
	inline ~ScopedTimer() { End(); }
};

} // namespace AzCore::Profiling

#define AZCORE_PROFILING_SCOPED_TIMER(scopeName)\
	static az::Profiling::AString _scopedTimerString(#scopeName);\
	static az::ClockTime _exceptionStart;\
	az::Profiling::ScopedTimer _scopedTimer(_scopedTimerString);

#define AZCORE_PROFILING_FUNC_TIMER()\
	static az::Profiling::AString _funcTimerString(__FUNCTION__);\
	static az::ClockTime _exceptionStart;\
	az::Profiling::ScopedTimer _funcTimer(_funcTimerString);

#define AZCORE_PROFILING_EXCEPTION_START()\
	_exceptionStart = az::Clock::now();

#define AZCORE_PROFILING_EXCEPTION_END() \
	az::Profiling::Exception(_scopedTimerString, az::Nanoseconds(az::Clock::now() - _exceptionStart));

#endif // AZCORE_PROFILING_HPP