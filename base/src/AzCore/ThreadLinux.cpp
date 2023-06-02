/*
	File: ThreadLinux.hpp
	Author: Philip Haynes
	The Posix-threads implementation of threads.
*/

#include "Thread.hpp"

#include <system_error>

#include <pthread.h>
#include <sys/sysinfo.h>

namespace AzCore {

struct ThreadData {
	pthread_t threadHandle;
};
static_assert(sizeof(ThreadData) <= 16);

ThreadData& GetThreadData(char *data) {
	return *(ThreadData*)data;
}

void Thread::_Launch(void* (*proc)(void*), void *call) {
	ThreadData &threadData = GetThreadData(data);
	int errnum = pthread_create(&threadData.threadHandle, nullptr, proc, call);
	if (errnum != 0) {
		threadData.threadHandle = 0;
		delete call;
		throw std::system_error(errnum, std::generic_category());
	}
}

Thread::Thread() {
	ThreadData &threadData = GetThreadData(data);
	threadData.threadHandle = 0;
}

Thread::Thread(Thread&& other) : threadHandle(other.threadHandle) {
	ThreadData &threadData = GetThreadData(data);
	ThreadData &otherThreadData = GetThreadData(other.data);
	threadData.threadHandle = otherThreadData.threadHandle;
	otherThreadData.threadHandle = 0;
}

bool Thread::Joinable() {
	ThreadData &threadData = GetThreadData(data);
	return threadData.threadHandle != 0;
}

void Thread::Join() {
	ThreadData &threadData = GetThreadData(data);
	if (threadData.threadHandle == 0)
		throw std::system_error(std::make_error_code(std::errc::no_such_process));
	pthread_join(threadData.threadHandle, nullptr);
	threadData.threadHandle = 0;
}

void Thread::Detach() {
	ThreadData &threadData = GetThreadData(data);
	if (!Joinable()) {
		throw std::system_error(std::make_error_code(std::errc::invalid_argument));
	}
	pthread_detach(threadData.threadHandle);
	threadData.threadHandle = 0;
}

Thread& Thread::operator=(Thread&& other) {
	ThreadData &threadData = GetThreadData(data);
	ThreadData &otherThreadData = GetThreadData(other.data);
	if (Joinable()) {
		throw std::system_error(std::make_error_code(std::errc::operation_in_progress));
	}
	threadData.threadHandle = otherThreadData.threadHandle;
	otherThreadData.threadHandle = 0;
	return *this;
}

static unsigned Thread::HardwareConcurrency() {
	return get_nprocs();
}

static void Thread::_Sleep(i64 nanoseconds) {
	struct timespec remaining = {
		(time_t)(nanoseconds / 1000000000),
		(long)(nanoseconds % 1000000000)
	};
	while (nanosleep(&remaining, &remaining) == -1 && errno == EINTR) {}
}

static void Thread::_SleepPrecise(i64 nanoseconds) {
	_Sleep(nanoseconds);
}

static void Thread::Yield() {
	sched_yield();
}

struct MutexData {
	pthread_mutex_t mutex;
};
static_assert(sizeof(MutexData) <= 48);

inline MutexData& GetMutexData(char *data) {
	return *(MutexData*)data;
}

Mutex::Mutex() {
	MutexData &mutexData = GetMutexData(data);
	pthread_mutex_init(&mutexData.mutex, nullptr);
}
Mutex::~Mutex() {
	MutexData &mutexData = GetMutexData(data);
	pthread_mutex_destroy(&mutexData.mutex);
}
void Mutex::Lock() {
	MutexData &mutexData = GetMutexData(data);
	pthread_mutex_lock(&mutexData.mutex);
}
void Mutex::Unlock() {
	MutexData &mutexData = GetMutexData(data);
	pthread_mutex_unlock(&mutexData.mutex);
}
bool Mutex::TryLock() {
	MutexData &mutexData = GetMutexData(data);
	return 0 == pthread_mutex_trylock(&mutexData.mutex);
}

} // namespace AzCore
