/*
	File: ThreadLinux.hpp
	Author: Philip Haynes
	The Posix-threads implementation of threads.
*/

#include "Thread.hpp"

#include <system_error>

#include <pthread.h>
#include <sys/sysinfo.h>
#include <cstring> // strerror

namespace AzCore {

struct ThreadData {
	pthread_t threadHandle;
};
static_assert(sizeof(ThreadData) <= 16);

ThreadData& GetThreadData(char *data) {
	return *(ThreadData*)data;
}

void Thread::_Launch(void* (*proc)(void*), void *call, void (*cleanup)(void*)) {
	ThreadData &threadData = GetThreadData(data);
	int errnum = pthread_create(&threadData.threadHandle, nullptr, proc, call);
	if (errnum != 0) {
		threadData.threadHandle = 0;
		cleanup(call);
		throw std::system_error(errnum, std::generic_category());
	}
}

Thread::Thread() {
	ThreadData &threadData = GetThreadData(data);
	threadData.threadHandle = 0;
}

Thread::Thread(Thread&& other) {
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

unsigned Thread::HardwareConcurrency() {
	return get_nprocs();
}


static inline void _SetProcessorAffinity(pthread_t threadHandle, SimpleRange<u16> cpus) {
	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	for (u16 cpu : cpus) {
		if (cpu >= CPU_SETSIZE) {
			// TODO: Support larger sets using CPU_ALLOC or maybe alloca
			fprintf(stderr, "Posix has a CPU set size limit of %u logical cores (tried to mask cpu %hu)", CPU_SETSIZE, cpu);
			continue;
		}
		CPU_SET(cpu, &cpuset);
	}
	if (int err = pthread_setaffinity_np(threadHandle, sizeof(cpuset), &cpuset)) {
		fprintf(stderr, "Failed to SetProcessorAffinity: %s (%i)", strerror(err), err);
	}
}

static inline void _ResetProcessorAffinity(pthread_t threadHandle) {
	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	u32 concurrency = Thread::HardwareConcurrency();
	for (u16 cpu = 0; cpu < concurrency; cpu++) {
		CPU_SET(cpu, &cpuset);
	}
	if (int err = pthread_setaffinity_np(threadHandle, sizeof(cpuset), &cpuset)) {
		fprintf(stderr, "Failed to ResetProcessorAffinity: %s (%i)", strerror(err), err);
	}
}

void Thread::SetProcessorAffinity(SimpleRange<u16> cpus) {
	pthread_t threadHandle = pthread_self();
	_SetProcessorAffinity(threadHandle, cpus);
}

void Thread::SetProcessorAffinity(Thread &thread, SimpleRange<u16> cpus) {
	ThreadData &threadData = GetThreadData(thread.data);
	_SetProcessorAffinity(threadData.threadHandle, cpus);
}

void Thread::ResetProcessorAffinity() {
	pthread_t threadHandle = pthread_self();
	_ResetProcessorAffinity(threadHandle);
}

void Thread::ResetProcessorAffinity(Thread &thread) {
	ThreadData &threadData = GetThreadData(thread.data);
	_ResetProcessorAffinity(threadData.threadHandle);
}

void Thread::_Sleep(i64 nanoseconds) {
	struct timespec remaining = {
		(time_t)(nanoseconds / 1000000000),
		(long)(nanoseconds % 1000000000)
	};
	while (nanosleep(&remaining, &remaining) == -1 && errno == EINTR) {}
}

void Thread::_SleepPrecise(i64 nanoseconds) {
	_Sleep(nanoseconds);
}

void Thread::Yield() {
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

struct CondVarData {
	pthread_cond_t conditionVariable;
};
static_assert(sizeof(CondVarData) <= AZCORE_CONDVAR_DATA_SIZE);

inline CondVarData& GetCondVarData(char *data) {
	return *(CondVarData*)data;
}

CondVar::CondVar() {
	CondVarData &condVarData = GetCondVarData(data);
	pthread_cond_init(&condVarData.conditionVariable, nullptr);
}

CondVar::~CondVar() {
	CondVarData &condVarData = GetCondVarData(data);
	pthread_cond_destroy(&condVarData.conditionVariable);
}

void CondVar::Wait(Mutex &mutex) {
	CondVarData &condVarData = GetCondVarData(data);
	MutexData &mutexData = GetMutexData(mutex.data);
	pthread_cond_wait(&condVarData.conditionVariable, &mutexData.mutex);
}

void CondVar::WakeOne() {
	CondVarData &condVarData = GetCondVarData(data);
	pthread_cond_signal(&condVarData.conditionVariable);
}

void CondVar::WakeAll() {
	CondVarData &condVarData = GetCondVarData(data);
	pthread_cond_broadcast(&condVarData.conditionVariable);
}

} // namespace AzCore
