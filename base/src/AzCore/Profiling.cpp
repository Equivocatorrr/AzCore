/*
	File: Profiling.cpp
	Author: Philip Haynes
*/

#include "profiling.hpp"
#include "IO/Log.hpp"
#include "Thread.hpp"
#include "QuickSort.hpp"

#define SAMPLE_MUTEX_LOCK 0

namespace AzCore::Profiling {

AZCORE_CREATE_STRING_ARENA_CPP()

static bool enabled = false;
static AStringMap<Nanoseconds> totalTimes;
static AStringMap<i32> timerDepth;
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

void Report() {
	if (!enabled) return;
	io::Log log("profiling.log", false, true);
	log.PrintLn("Total samples: ", nSamples, ", exceptions: ", nExceptions);
#if SAMPLE_MUTEX_LOCK
	log.PrintLn("Avg. mutex.Lock() wait time: ", FormatTime(totalTimes[mutexLockScope] / nMutexLocks), " with ", nMutexLocks, " total locks.");
#endif
	using Node_t = decltype(*totalTimes.begin());
	Array<Node_t> nodes;
	for (const auto &node : totalTimes) {
		nodes.Append(node);
	}
	if (nodes.size == 0) return;
	Nanoseconds totalRuntime = Clock::now() - programStart;
	log.PrintLn("Total program runtime: ", FormatTime(totalRuntime));
	QuickSort(nodes, [](Node_t lhs, Node_t rhs) { return *lhs.value > *rhs.value; });
	for (const auto &node : nodes) {
		f64 percent = (f64)node.value->count() / (f64)totalRuntime.count() * 100.0;
		log.PrintLn(node.key, " ", AlignText(48), FormatFloat(percent, 10, 4), "% ", AlignText(60), FormatTime(*node.value));
	}
}

void Exception(AString scopeName, Nanoseconds time) {
	if (!enabled) return;
#if SAMPLE_MUTEX_LOCK
	ClockTime start = Clock::now();
	mutex.Lock();
	totalTimes[mutexLockScope] += Clock::now() - start;
	nMutexLocks++;
#else
	mutex.Lock();
#endif
	nExceptions++;
	totalTimes[scopeName] -= time;
	mutex.Unlock();
}

Timer::Timer(AString scopeName) : scope(scopeName) {}

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
	Nanoseconds time = Clock::now() - start;
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
		totalTimes[scope] += time;
	}
	timerDepth[scope]--;
	mutex.Unlock();
}

} // namespace AzCore::Profiling