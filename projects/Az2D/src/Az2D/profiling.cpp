/*
	File: profiling.cpp
	Author: Philip Haynes
*/

#include "profiling.hpp"
#include "AzCore/IO/Log.hpp"
#include "AzCore/Thread.hpp"
#include "AzCore/QuickSort.hpp"

namespace Az2D::Profiling {

AZCORE_CREATE_STRING_ARENA_CPP()

bool enabled = false;
AStringMap<az::Nanoseconds> totalTimes;
az::Mutex mutex;
az::ClockTime programStart = az::Clock::now();

void Enable() {
	enabled = true;
}

void Report() {
	if (!enabled) return;
	az::io::Log log("profiling.log", false, true);
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
	mutex.Lock();
	totalTimes[scopeName] -= time;
	mutex.Unlock();
}

ScopedTimer::ScopedTimer(AString scopeName) : scope(scopeName), start(az::Clock::now()) {}

ScopedTimer::~ScopedTimer() {
	if (!enabled) return;
	az::ClockTime end = az::Clock::now();
	mutex.Lock();
	az::Nanoseconds &ns = totalTimes[scope];
	ns += az::Nanoseconds(end-start);
	mutex.Unlock();
}

} // namespace Az2D::Profiling