/*
	File: profiling.cpp
	Author: Philip Haynes
*/

#include "profiling.hpp"
#include "AzCore/IO/Log.hpp"
#include "AzCore/Thread.hpp"
#include "AzCore/QuickSort.hpp"

#define SAMPLE_MUTEX_LOCK 0

namespace Az2D::Profiling {

AZCORE_CREATE_STRING_ARENA_CPP()

static bool enabled = false;
static AStringMap<az::Nanoseconds> totalTimes;
static AStringMap<i32> timerDepth;
static i64 nSamples = 0;
static i64 nExceptions = 0;
static az::Mutex mutex;
static az::ClockTime programStart = az::Clock::now();

void Enable() {
	enabled = true;
}

#if SAMPLE_MUTEX_LOCK
static AString mutexLockScope = "Az2D::Profiling::Timer mutex.Lock()";
static i64 nMutexLocks = 0;
#endif

void Report() {
	if (!enabled) return;
	az::io::Log log("profiling.log", false, true);
	log.PrintLn("Total samples: ", nSamples, ", exceptions: ", nExceptions);
#if SAMPLE_MUTEX_LOCK
	log.PrintLn("Avg. mutex.Lock() wait time: ", az::FormatTime(totalTimes[mutexLockScope] / nMutexLocks), " with ", nMutexLocks, " total locks.");
#endif
	using Node_t = decltype(*totalTimes.begin());
	az::Array<Node_t> nodes;
	for (const auto &node : totalTimes) {
		nodes.Append(node);
	}
	if (nodes.size == 0) return;
	az::Nanoseconds totalRuntime = az::Clock::now() - programStart;
	log.PrintLn("Total program runtime: ", az::FormatTime(totalRuntime));
	az::QuickSort(nodes, [](Node_t lhs, Node_t rhs) { return *lhs.value > *rhs.value; });
	for (const auto &node : nodes) {
		f64 percent = (f64)node.value->count() / (f64)totalRuntime.count() * 100.0;
		log.PrintLn(node.key, " ", az::AlignText(48), az::FormatFloat(percent, 10, 4), "% ", az::AlignText(60), az::FormatTime(*node.value));
	}
}

void Exception(AString scopeName, az::Nanoseconds time) {
	if (!enabled) return;
#if SAMPLE_MUTEX_LOCK
	az::ClockTime start = az::Clock::now();
	mutex.Lock();
	totalTimes[mutexLockScope] += az::Clock::now() - start;
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
	start = az::Clock::now();
#if SAMPLE_MUTEX_LOCK
	mutex.Lock();
	totalTimes[mutexLockScope] += az::Clock::now() - start;
	nMutexLocks++;
#else
	mutex.Lock();
#endif
	timerDepth.ValueOf(scope, 0)++;
	mutex.Unlock();
}

void Timer::End() {
	if (!enabled) return;
	az::Nanoseconds time = az::Clock::now() - start;
#if SAMPLE_MUTEX_LOCK
	az::ClockTime mutexStart = az::Clock::now();
	mutex.Lock();
	totalTimes[mutexLockScope] += az::Clock::now() - mutexStart;
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

ScopedTimer::ScopedTimer(AString scopeName) : Timer(scopeName) {
	Start();
}

ScopedTimer::~ScopedTimer() {
	End();
}

} // namespace Az2D::Profiling