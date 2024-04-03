#ifndef AZCORE_ASSERT_HPP
#define AZCORE_ASSERT_HPP

#include "basictypes.hpp"

namespace AzCore {

namespace io {

class Log;
extern Log cerr;

} // namespace io

void PrintBacktrace(io::Log &log);
inline void PrintBacktrace() {
	PrintBacktrace(io::cerr);
}
void _AssertFailure(const char *file, const char *line, const char *message);
constexpr auto* _GetFileName(const char* const path) {
	const auto* startPosition = path;
	for (const auto* cur = path; *cur != '\0'; ++cur) {
		if (*cur == '\\' || *cur == '/') startPosition = cur+1;
	}
	return startPosition;
}

} // namespace AzCore


#define STRINGIFY_DAMMIT(x) #x
#define STRINGIFY(x) STRINGIFY_DAMMIT(x)
#ifdef NDEBUG
	#define AzAssert(condition, message)
#else
	#define AzAssert(condition, message) if (!(condition)) {AzCore::_AssertFailure(AzCore::_GetFileName(__FILE__), STRINGIFY(__LINE__), (message));}
#endif
// Assert that persists in release mode
#define AzAssertRel(condition, message) if (!(condition)) {AzCore::_AssertFailure(AzCore::_GetFileName(__FILE__), STRINGIFY(__LINE__), (message));}

#endif // AZCORE_ASSERT_HPP

// Do this so if we're included without AZCORE_DEFINE_ASSERT,
// and it gets defined later, we can still define Assert
#if defined(AZCORE_DEFINE_ASSERT) && !defined(Assert)
	#define Assert(condition, message) AzAssert(condition, message)
#endif // AZCORE_DEFINE_ASSERT