/*
	File: Profiling.cpp
	Author: Philip Haynes
*/

#include "Profiling.hpp"
#include "IO/Log.hpp"
#include "Thread.hpp"
#include "QuickSort.hpp"

#define SAMPLE_MUTEX_LOCK 0

namespace AzCore::Profiling {

AZCORE_CREATE_STRING_ARENA_CPP()

struct TimeInfo {
	Nanoseconds totalTime = Nanoseconds(0);
	Nanoseconds totalTimeExceptions = Nanoseconds(0);
	Nanoseconds minTime = Nanoseconds(INT64_MAX);
	Nanoseconds maxTime = Nanoseconds(0);
	i32 numSamples = 0;
	i32 numExceptions = 0;
};

static bool enabled = false;
static AStringMap<TimeInfo> timeInfos;
static thread_local AStringMap<i32> timerDepth;
static i64 nSamples = 0;
static i64 nExceptions = 0;
static Mutex mutex;
static ClockTime programStart = Clock::now();

void Enable() {
	enabled = true;
}

#if SAMPLE_MUTEX_LOCK
static AString mutexLockScope = "AzCore::Profiling::Timer mutex.Lock()";
static i64 nMutexLocks = 0;
#endif

void Report(bool pretty) {
	if (!enabled) return;
	Nanoseconds totalRuntime = Clock::now() - programStart;
	io::Log log("profiling.csv", false, true);
	log.PrintLn("Samples,", AlignText(12), "Exceptions,", AlignText(24), "Runtime");
	log.PrintLn(nSamples, ",", AlignText(12), nExceptions, ",", AlignText(24), FormatTime(totalRuntime));
	log.Newline();
#if SAMPLE_MUTEX_LOCK
	log.PrintLn("Avg. mutex.Lock() wait time: ", FormatTime(totalTimes[mutexLockScope] / nMutexLocks), " with ", nMutexLocks, " total locks.");
	log.Newline();
#endif
	using Node_t = decltype(*timeInfos.begin());
	i32 maxNameLen = 0;
	Array<Node_t> nodes;
	for (const auto &node : timeInfos) {
		i32 nameLen = node.key.GetString().size;
		if (nameLen > maxNameLen) maxNameLen = nameLen;
		nodes.Append(node);
	}
	if (nodes.size == 0) return;
	if (pretty) {
		log.PrintLn("Scope,", AlignText(maxNameLen + 2), "% of Runtime,", AlignText(maxNameLen + 16), "Time,", AlignText(maxNameLen + 16 + 36), "Exception Time,", AlignText(maxNameLen + 16 + 36*2), "Minimum Time,", AlignText(maxNameLen + 16 + 36*3), "Average Time,", AlignText(maxNameLen + 16 + 36*4), "Maximum Time,", AlignText(maxNameLen + 16 + 36*5), "Num Samples,", AlignText(maxNameLen + 16 + 36*6 + 16), "Num Exceptions");
	} else {
		log.PrintLn("Scope, % of Runtime, Time, Exception Time, Minimum Time, Average Time, Maximum Time, Num Samples, Num Exceptions");
	}
	QuickSort(nodes, [](Node_t lhs, Node_t rhs) { return lhs.value->totalTime > rhs.value->totalTime; });
	for (const auto &node : nodes) {
		f64 percent = (f64)node.value->totalTime.count() / (f64)totalRuntime.count() * 100.0;
		Nanoseconds averageTime = node.value->totalTime / node.value->numSamples;
		if (pretty) {
			log.PrintLn(node.key, ",", AlignText(maxNameLen + 2), FormatFloat(percent, 10, 4), "%,", AlignText(maxNameLen + 16), FormatTime(node.value->totalTime), ",", AlignText(maxNameLen + 16 + 36), FormatTime(node.value->totalTimeExceptions), ",", AlignText(maxNameLen + 16 + 36*2), FormatTime(node.value->minTime), ",", AlignText(maxNameLen + 16 + 36*3), FormatTime(averageTime), ",", AlignText(maxNameLen + 16 + 36*4), FormatTime(node.value->maxTime), ",", AlignText(maxNameLen + 16 + 36*5), node.value->numSamples, ",", AlignText(maxNameLen + 16 + 36*6 + 16), node.value->numExceptions);
		} else {
			log.PrintLn(node.key, ",", FormatFloat(percent, 10, 4), "%,", FormatTime(node.value->totalTime), ",", FormatTime(node.value->totalTimeExceptions), ",", FormatTime(node.value->minTime), ",", FormatTime(averageTime), ",", FormatTime(node.value->maxTime), ",", node.value->numSamples, ",", node.value->numExceptions);
		}
	}
}

Timer::Timer(AString scopeName) : scope(scopeName), exceptionTime(0) {}

void Timer::Start() {
	if (!enabled) return;
	start = Clock::now();
#if SAMPLE_MUTEX_LOCK
	mutex.Lock();
	totalTimes[mutexLockScope] += Clock::now() - start;
	nMutexLocks++;
#else
	mutex.Lock();
#endif
	timerDepth.ValueOf(scope, 0)++;
	mutex.Unlock();
}

void Timer::End() {
	if (!enabled) return;
	Nanoseconds time = Clock::now() - start - exceptionTime;
#if SAMPLE_MUTEX_LOCK
	ClockTime mutexStart = Clock::now();
	mutex.Lock();
	totalTimes[mutexLockScope] += Clock::now() - mutexStart;
	nMutexLocks++;
#else
	mutex.Lock();
#endif
	if (timerDepth[scope] == 1) {
		nSamples++;
		TimeInfo &timeInfo = timeInfos[scope];
		timeInfo.numSamples++;
		timeInfo.totalTime += time;
		if (time > timeInfo.maxTime) timeInfo.maxTime = time;
		if (time < timeInfo.minTime) timeInfo.minTime = time;
	}
	timerDepth[scope]--;
	mutex.Unlock();
}

void Timer::ExceptionStart() {
	if (!enabled) return;
#if SAMPLE_MUTEX_LOCK
	ClockTime start = Clock::now();
	mutex.Lock();
	totalTimes[mutexLockScope] += Clock::now() - start;
	nMutexLocks++;
#else
	mutex.Lock();
#endif
	exceptionStart = Clock::now();
	mutex.Unlock();
}

void Timer::ExceptionEnd() {
	if (!enabled) return;
	Nanoseconds time = Clock::now() - exceptionStart;
	exceptionTime += time;
#if SAMPLE_MUTEX_LOCK
	ClockTime start = Clock::now();
	mutex.Lock();
	totalTimes[mutexLockScope] += Clock::now() - start;
	nMutexLocks++;
#else
	mutex.Lock();
#endif
	nExceptions++;
	TimeInfo &timeInfo = timeInfos[scope];
	timeInfo.totalTimeExceptions += time;
	timeInfo.numExceptions++;
	mutex.Unlock();
}

} // namespace AzCore::Profiling