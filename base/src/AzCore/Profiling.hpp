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
void Report(bool pretty=false);

class Timer {
protected:
	AString scope;
	az::ClockTime start;
	az::ClockTime exceptionStart;
	az::Nanoseconds exceptionTime;
public:
	Timer(AString scopeName);
	void Start();
	void End();
	void ExceptionStart();
	void ExceptionEnd();
};

class ScopedTimer : public Timer {
public:
	inline ScopedTimer(AString scopeName) : Timer(scopeName) { Start(); }
	inline ~ScopedTimer() { End(); }
};

} // namespace AzCore::Profiling

#define AZCORE_PROFILING_SCOPED_TIMER(scopeName)\
	static az::Profiling::AString _scopedTimerString(#scopeName);\
	az::Profiling::ScopedTimer _timer(_scopedTimerString);

constexpr az::Str functionNameFromPrettyFunction(const char *pretty) {
	az::Str out = pretty;
	for (i32 i = 1; i < out.size; i++) {
		if (out[i-1] == ' ') {
			out.str += i;
			out.size -= i;
			break;
		}
	}
	for (i32 i = 1; i < out.size; i++) {
		if (out[i] == '(') {
			out.size = i;
			break;
		}
	}
	return out;
}

#define AZCORE_PROFILING_FUNC_TIMER()\
	static az::Profiling::AString _funcTimerString(functionNameFromPrettyFunction(AZCORE_PRETTY_FUNCTION));\
	az::Profiling::ScopedTimer _timer(_funcTimerString);

#define AZCORE_PROFILING_EXCEPTION_START()\
	_timer.ExceptionStart();

#define AZCORE_PROFILING_EXCEPTION_END() \
	_timer.ExceptionEnd();

#endif // AZCORE_PROFILING_HPP