/*
	File: Thread.cpp
	Author: Philip Haynes
*/

#include <stdlib.h>

#ifdef __unix
	#include "ThreadLinux.cpp"
#elif defined(_WIN32)
	#include "ThreadWin32.cpp"
#else
	#error "Threads haven't been implemented for this platform."
#endif

void exit_thread_safe(int code) {
	static az::Mutex mutex;
	mutex.Lock();
	exit(code);
}