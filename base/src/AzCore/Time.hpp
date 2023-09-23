/*
	File: Time.hpp
	Author: Philip Haynes
*/

#ifndef AZCORE_TIME_HPP
#define AZCORE_TIME_HPP

#include "basictypes.hpp"
#include <chrono>
#include "Memory/String.hpp"
#include "Memory/StaticArray.hpp"

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

// Counts frametimes and gives you meaningful information about the last 30 frames.
// times are measured in milliseconds
struct FrametimeCounter {
	static constexpr i32 totalFrames = 30;
	StaticArray<f32, totalFrames> frametimes = StaticArray<f32, totalFrames>(totalFrames, 16.6666f);
	i32 frame = 0;
	ClockTime lastTime = Clock::now();
	inline void Update() {
		ClockTime time = Clock::now();
		i64 nanoseconds = Nanoseconds(time-lastTime).count();
		f32 milliseconds = f32((f64)nanoseconds / 1000000.0);
		lastTime = time;
		frametimes[frame] = milliseconds;
		frame = (frame + 1) % totalFrames;
	}
	inline f32 Average() const {
		f32 total = 0.0f;
		for (f32 time : frametimes) {
			total += time;
		}
		return total / (f32)totalFrames;
	}
	inline f32 AverageWithoutOutliers() const {
		f32 total = 0.0f;
		i32 count = 0;
		for (f32 time : frametimes) {
			if (time < 1000.0f / 15.0f) {
				total += time;
				count++;
			}
		}
		if (count == 0) return Average();
		return total / (f32)count;
	}
	inline f32 Max() const {
		f32 max = 0.0f;
		for (f32 time : frametimes) {
			if (time > max) max = time;
		}
		return max;
	}
	inline f32 Min() const {
		f32 min = 1000000000.0f;
		for (f32 time : frametimes) {
			if (time < min) min = time;
		}
		return min;
	}
};

} // namespace AzCore

#endif // AZCORE_TIME_HPP
