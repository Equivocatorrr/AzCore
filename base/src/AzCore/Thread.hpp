/*
	File: Thread.hpp
	Author: Philip Haynes
	Just a cross-platform implementation of threads because cross-compiling with mingw and threads is a mess.
*/

#ifndef AZCORE_THREAD_HPP
#define AZCORE_THREAD_HPP

#ifdef __unix
	#include "ThreadLinux.hpp"
#elif defined(_WIN32)
	#include "ThreadWin32.hpp"
#else
	#error "Threads haven't been implemented for this platform."
#endif

#endif // AZCORE_THREAD_HPP