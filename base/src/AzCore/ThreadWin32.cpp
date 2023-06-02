/*
	File: ThreadWin32.hpp
	Author: Philip Haynes
	The Windows implementation of threads.
*/

#include "Thread.hpp"

#include <system_error>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <synchapi.h>
#include <handleapi.h>
#include <sysinfoapi.h>
#include <processthreadsapi.h>
#include <process.h>
#if defined(_MSC_VER)
	#include <timeapi.h>
#endif

#include <cstdio>

namespace AzCore {

struct ThreadData {
	HANDLE threadHandle;
	DWORD id;
};
static_assert(sizeof(ThreadData) <= 16);

ThreadData& GetThreadData(char *data) {
	return *(ThreadData*)data;
}

void Thread::_Launch(unsigned (__stdcall *proc)(void*), void *call) {
	ThreadData &threadData = GetThreadData(data);
	auto handle = _beginthreadex(NULL, 0, proc, call, 0, (unsigned*)&threadData.id);
	if (handle == 0) {
		threadData.threadHandle = 0;
		int errnum = errno;
		delete call;
		throw std::system_error(errnum, std::generic_category());
	} else {
		threadData.threadHandle = reinterpret_cast<HANDLE>(handle);
	}
}

Thread::Thread() {
	ThreadData &threadData = GetThreadData(data);
	threadData.threadHandle = 0;
	threadData.id = 0;
}

Thread::Thread(Thread&& other) {
	ThreadData &threadData = GetThreadData(data);
	ThreadData &otherThreadData = GetThreadData(other.data);
	threadData.threadHandle = otherThreadData.threadHandle;
	threadData.id = otherThreadData.id;
	otherThreadData.threadHandle = 0;
	otherThreadData.id = 0;
}

bool Thread::Joinable() {
	ThreadData &threadData = GetThreadData(data);
	return threadData.threadHandle != 0;
}

void Thread::Join() {
	ThreadData &threadData = GetThreadData(data);
	if (threadData.id == GetCurrentThreadId())
		throw std::system_error(std::make_error_code(std::errc::resource_deadlock_would_occur));
	if (threadData.threadHandle == 0)
		throw std::system_error(std::make_error_code(std::errc::no_such_process));
	WaitForSingleObject(threadData.threadHandle, 0xfffffff1);
	CloseHandle(threadData.threadHandle);
	threadData.threadHandle = 0;
	threadData.id = 0;
}

void Thread::Detach() {
	ThreadData &threadData = GetThreadData(data);
	if (!Joinable()) {
		throw std::system_error(std::make_error_code(std::errc::invalid_argument));
	}
	CloseHandle(threadData.threadHandle);
	threadData.threadHandle = 0;
	threadData.id = 0;
}

Thread& Thread::operator=(Thread&& other) {
	ThreadData &threadData = GetThreadData(data);
	ThreadData &otherThreadData = GetThreadData(other.data);
	if (Joinable()) {
		throw std::system_error(std::make_error_code(std::errc::operation_in_progress));
	}
	threadData.threadHandle = otherThreadData.threadHandle;
	threadData.id = otherThreadData.id;
	otherThreadData.threadHandle = 0;
	otherThreadData.id = 0;
	return *this;
}

unsigned Thread::HardwareConcurrency() {
	SYSTEM_INFO sysinfo;
	::GetNativeSystemInfo(&sysinfo);
	return sysinfo.dwNumberOfProcessors;
}

void Thread::_Sleep(i64 nanoseconds) {
	AZ_MSVC_ONLY(timeBeginPeriod(1));
	::Sleep((DWORD)(nanoseconds/1000000));
	AZ_MSVC_ONLY(timeEndPeriod(1));
}

void Thread::_SleepPrecise(i64 nanoseconds) {
	static bool failedOnce = false;
	static ::HANDLE timer;
	static bool madeTimer = false;
	DWORD result;
	
	if (failedOnce) goto fallback;
	
	if (!madeTimer) {
		if (!(timer = ::CreateWaitableTimer(NULL, TRUE, NULL))) goto failure;
		// We don't need to close the handle because it will be closed automatically on process shutdown
		madeTimer = true;
	}
	
	::LARGE_INTEGER time;
	time.QuadPart = -nanoseconds/100;
	if (!SetWaitableTimer(timer, &time, 0, NULL, NULL, FALSE)) {
		goto failure;
	}
	AZ_MSVC_ONLY(timeBeginPeriod(1));
	result = WaitForSingleObject(timer, INFINITE);
	AZ_MSVC_ONLY(timeEndPeriod(1));
	if (result == WAIT_FAILED) goto failure;
	return;
failure:
	if (!failedOnce) {
		fprintf(stderr, "SleepPrecise failed! Falling back to Sleep");
	}
	failedOnce = true;
fallback:
	_Sleep(nanoseconds);
}

#ifdef Yield
#undef Yield
#endif // I don't know who the fuck did this but I'm gonna scream I swear to god
inline void Thread::Yield() {
	::Sleep(0);
}

struct MutexData {
	CRITICAL_SECTION criticalSection;
};
static_assert(sizeof(MutexData) <= 48);

inline MutexData& GetMutexData(char *data) {
	return *(MutexData*)data;
}

Mutex::Mutex() {
	MutexData &mutexData = GetMutexData(data);
	InitializeCriticalSection(&mutexData.criticalSection);
}
Mutex::~Mutex() {
	MutexData &mutexData = GetMutexData(data);
	DeleteCriticalSection(&mutexData.criticalSection);
}
void Mutex::Lock() {
	MutexData &mutexData = GetMutexData(data);
	EnterCriticalSection(&mutexData.criticalSection);
}
void Mutex::Unlock() {
	MutexData &mutexData = GetMutexData(data);
	LeaveCriticalSection(&mutexData.criticalSection);
}
bool Mutex::TryLock() {
	MutexData &mutexData = GetMutexData(data);
	BOOL ret = TryEnterCriticalSection(&mutexData.criticalSection);
	return ret;
}

} // namespace AzCore
