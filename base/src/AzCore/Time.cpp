/*
	File: Time.cpp
	Author: Philip Haynes
*/

#include "Time.hpp"

namespace AzCore {

template<u32 lastUnit=5>
force_inline(void)
_AppendToString(String &string, Nanoseconds time) {
	static_assert(lastUnit <= 5);
	u64 count = time.count();
	const u64 unitTimes[] = {UINT64_MAX, (u64)3600000000000, (u64)60000000000, (u64)1000000000, (u64)1000000, (u64)1000, (u64)1};
#ifdef _WIN32
	// Because Windows consoles don't handle the 'μ' character correctly for some reason.
	const char *unitStrings[] = {"h", "m", "s", "ms", "us", "ns"};
#else
	const char *unitStrings[] = {"h", "m", "s", "ms", "μs", "ns"};
#endif
	bool addSpace = false;
	for (u32 i = 0; i <= lastUnit; i++) {
		if (count >= unitTimes[i+1]) {
			if (addSpace) {
				AppendToString(string, ' ');
			}
			AppendToString(string, (count%unitTimes[i])/unitTimes[i+1]);
			AppendToString(string, unitStrings[i]);
			addSpace = true;
		}
	}
	if (!addSpace) {
		AppendMultipleToString(string, '0', unitStrings[lastUnit]);
	}
}

void AppendToString(String &string, Nanoseconds time) {
	_AppendToString<5>(string, time);
}

void AppendToString(String &string, Microseconds time) {
	_AppendToString<4>(string, Nanoseconds(time));
}

void AppendToString(String &string, Milliseconds time) {
	_AppendToString<3>(string, Nanoseconds(time));
}

void AppendToString(String &string, Seconds time) {
	_AppendToString<2>(string, Nanoseconds(time));
}

void AppendToString(String &string, Minutes time) {
	_AppendToString<1>(string, Nanoseconds(time));
}

void AppendToString(String &string, Hours time) {
	_AppendToString<0>(string, Nanoseconds(time));
}


String FormatTime(Nanoseconds time) {
	String out;
	AppendToString(out, time);
	return out;
}

} // namespace AzCore