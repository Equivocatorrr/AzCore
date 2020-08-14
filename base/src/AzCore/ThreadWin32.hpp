/*
    File: ThreadWin32.hpp
    Author: Philip Haynes
    The Windows implementation of threads.
*/

#include <tuple>
#include <functional>
#include <memory>
#include <system_error>
#include <chrono>

#include <atomic>

#include <synchapi.h>
#include <handleapi.h>
#include <sysinfoapi.h>
#include <processthreadsapi.h>
#include <process.h>

namespace AzCore {

namespace ThreadStuff {

// Nobody should use this language...
template<std::size_t...>
struct IntSequence {};
template<std::size_t N, std::size_t... S>
struct IntSequenceGen : IntSequenceGen<N-1, N-1, S...> { };
template<std::size_t... S>
struct IntSequenceGen<0, S...> { typedef IntSequence<S...> type; };
// If that ain't the most confusing way to get the first N natural numbers...

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
    HANDLE threadHandle;
    DWORD id;

    template<class Call>
    static unsigned __stdcall threadProc(void *ptr) {
        std::unique_ptr<Call> call(static_cast<Call*>(ptr));
        call->call();
        return 0;
    }

public:

    template<class Function, typename... Arguments>
    explicit Thread(Function&& function, Arguments&&... arguments) {
        static_assert(std::is_invocable<typename std::decay<Function>::type, typename std::decay<Arguments>::type...>::value);
        typedef ThreadStuff::FunctionCaller<Function, Arguments...> Call;
        auto call = new Call(std::forward<Function>(function), std::forward<Arguments>(arguments)...);
        auto handle = _beginthreadex(NULL, 0, threadProc<Call>, static_cast<LPVOID>(call), 0, reinterpret_cast<unsigned*>(&id));
        if (handle == 0) {
            threadHandle = 0;
            int errnum = errno;
            delete call;
            throw std::system_error(errnum, std::generic_category());
        } else {
            threadHandle = reinterpret_cast<HANDLE>(handle);
        }
    }

    Thread() : threadHandle(0), id(0) {}

    Thread(const Thread& other) = delete;

    Thread(Thread&& other) : threadHandle(other.threadHandle), id(other.id) {
        other.threadHandle = 0;
        other.id = 0;
    }

    inline bool Joinable() {
        return threadHandle != 0;
    }

    void Join() {
        if (id == GetCurrentThreadId())
            throw std::system_error(std::make_error_code(std::errc::resource_deadlock_would_occur));
        if (threadHandle == 0)
            throw std::system_error(std::make_error_code(std::errc::no_such_process));
        WaitForSingleObject(threadHandle, 0xfffffff1);
        CloseHandle(threadHandle);
        threadHandle = 0;
        id = 0;
    }

    void Detach() {
        if (!Joinable()) {
            throw std::system_error(std::make_error_code(std::errc::invalid_argument));
        }
        CloseHandle(threadHandle);
        threadHandle = 0;
        id = 0;
    }

    ~Thread() {
        if (Joinable()) {
            std::terminate();
        }
    }

    Thread& operator=(const Thread& other) = delete;
    Thread& operator=(Thread&& other) {
        if (Joinable()) {
            std::terminate();
        }
        threadHandle = other.threadHandle;
        id = other.id;
        other.threadHandle = 0;
        other.id = 0;
        return *this;
    }

    static unsigned HardwareConcurrency() {
        SYSTEM_INFO sysinfo;
        ::GetNativeSystemInfo(&sysinfo);
        return sysinfo.dwNumberOfProcessors;
    }

    template<class Rep, class Period>
    static void Sleep(const std::chrono::duration<Rep,Period>& duration) {
        ::Sleep((DWORD)std::chrono::duration_cast<std::chrono::milliseconds>(duration).count());
    }

    #ifdef Yield
    #undef Yield
    #endif // I don't know who the fuck did this but I'm gonna scream I swear to god
    static inline void Yield() {
        ::Sleep(0);
    }
};

class Mutex {
    CRITICAL_SECTION criticalSection;
public:
    inline Mutex() {
        InitializeCriticalSection(&criticalSection);
    }
    inline ~Mutex() {
        DeleteCriticalSection(&criticalSection);
    }
    inline void Lock() {
        EnterCriticalSection(&criticalSection);
    }
    inline void Unlock() {
        LeaveCriticalSection(&criticalSection);
    }
    inline bool TryLock() {
        BOOL ret = TryEnterCriticalSection(&criticalSection);
        return ret;
    }
};

} // namespace AzCore
