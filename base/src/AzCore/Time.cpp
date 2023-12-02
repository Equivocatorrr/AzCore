/*
	File: Time.cpp
	Author: Philip Haynes
*/

#include "Time.hpp"

namespace AzCore {

String FormatTime(Nanoseconds time) {
	String out;
	u64 count = time.count();
	const u64 unitTimes[] = {UINT64_MAX, 60000000000, 1000000000, 1000000, 1000, 1};
	const char *unitStrings[] = {"m", "s", "ms", "μs", "ns"};
	bool addSpace = false;
	for (u32 i = 0; i < 5; i++) {
		if (count > unitTimes[i+1]) {
			if (addSpace) {
				out += ' ';
			}
			AppendToString(out, (count%unitTimes[i])/unitTimes[i+1]);
			out.Append(unitStrings[i]);
			addSpace = true;
		}
	}
	if (out.size == 0) out = "0";
	return out;
}

} // namespace AzCore