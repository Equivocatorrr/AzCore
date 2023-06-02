/*
	File: Thread.hpp
	Author: Philip Haynes
	Just a cross-platform implementation of threads that uses threads native to the platforms.
*/

#ifndef AZCORE_THREAD_HPP
#define AZCORE_THREAD_HPP

#include "basictypes.hpp"
#include <tuple>
#include <functional>
#include <chrono>

namespace AzCore {

namespace ThreadStuff {

template<std::size_t...>
struct IntSequence {};
template<std::size_t N, std::size_t... S>
struct IntSequenceGen : IntSequenceGen<N-1, N-1, S...> { };
template<std::size_t... S>
struct IntSequenceGen<0, S...> { typedef IntSequence<S...> type; };

template<class Function, typename... Arguments>
class FunctionCaller {
	std::tuple<typename std::decay<Arguments>::type...> args;
	typename std::decay<Function>::type func;

	template<std::size_t... S>
	void call(IntSequence<S...>) {
		std::invoke(std::move(func), std::move(std::get<S>(args))...);
	}
public:
	explicit FunctionCaller(Function&& function, Arguments&&... arguments) :
		args(std::forward<Arguments>(arguments)...), func(std::forward<Function>(function)) {}
	void call() {
		call(typename IntSequenceGen<sizeof...(Arguments)>::type());
	}
};

} // namespace ThreadStuff

class Thread {
	alignas(8) char data[16];
	
	// This distinction MUST exist in the headers because the alternative is doing 2 heap allocations per thread launch instead of just the 1 we're already doing for the closure.
#ifdef __unix
	template<class Call>
	static void* threadProc(void *ptr) {
		Call *call = (Call*)ptr;
		call->call();
		delete call;
		return nullptr;
	}
	
	void _Launch(void* (*proc)(void*), void *call);
#elif defined(_WIN32)
	template<class Call>
	static unsigned __stdcall threadProc(void *ptr) {
		Call *call = (Call*)ptr;
		call->call();
		delete call;
		return 0;
	}
	
	void _Launch(unsigned (__stdcall *proc)(void*), void *call);
#endif

public:

	template<class Function, typename... Arguments>
	explicit Thread(Function&& function, Arguments&&... arguments) {
		static_assert(std::is_invocable<typename std::decay<Function>::type, typename std::decay<Arguments>::type...>::value);
		typedef ThreadStuff::FunctionCaller<Function, Arguments...> Call;
		auto call = new Call(std::forward<Function>(function), std::forward<Arguments>(arguments)...);
		_Launch(threadProc<Call>, (void*)call);
	}

	Thread();

	Thread(const Thread& other) = delete;

	Thread(Thread&& other);

	bool Joinable();

	void Join();

	void Detach();

	inline ~Thread() {
		if (Joinable()) {
			fprintf(stderr, "Tried to destruct a thread that's still joinable!\n");
			std::terminate();
		}
	}

	Thread& operator=(const Thread& other) = delete;
	Thread& operator=(Thread&& other);

	static unsigned HardwareConcurrency();

	template<class Rep, class Period>
	static inline void Sleep(const std::chrono::duration<Rep,Period>& duration) {
		auto nano = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
		_Sleep(nano.count());
	}
	// This does the same thing as Sleep on Linux, but it's different on Windows. Results may vary.
	template<class Rep, class Period>
	static inline void SleepPrecise(const std::chrono::duration<Rep,Period>& duration) {
		auto nano = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
		_SleepPrecise(nano.count());
	}
	static void _Sleep(i64 nanoseconds);
	static void _SleepPrecise(i64 nanoseconds);

	static void Yield();
};

class Mutex {
	alignas(8) char data[48];
public:
	Mutex();
	~Mutex();
	void Lock();
	void Unlock();
	bool TryLock();
};

} // namespace AzCore

#endif // AZCORE_THREAD_HPP