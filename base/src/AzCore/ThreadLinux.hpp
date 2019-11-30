/*
    File: ThreadLinux.hpp
    Author: Philip Haynes
    The Posix-threads implementation of threads.
*/

#include <tuple>
#include <functional>
#include <memory>
#include <system_error>
#include <chrono>

#include <atomic>

#include <pthread.h>

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
    pthread_t threadHandle;
    
    template<class Call>
    static void* threadProc(void *ptr) {
        std::unique_ptr<Call> call(static_cast<Call*>(ptr));
        call->call();
        return nullptr;
    }

public:

    template<class Function, typename... Arguments>
    explicit Thread(Function&& function, Arguments&&... arguments) {
        static_assert(std::is_invocable<typename std::decay<Function>::type, typename std::decay<Arguments>::type...>::value);
        typedef ThreadStuff::FunctionCaller<Function, Arguments...> Call;
        auto call = new Call(std::forward<Function>(function), std::forward<Arguments>(arguments)...);
        int errnum = pthread_create(&threadHandle, nullptr, threadProc<Call>, (void*)call);
        if (errnum != 0) {
            threadHandle = 0;
            delete call;
            throw std::system_error(errnum, std::generic_category());
        }
    }

    Thread() : threadHandle(0) {}

    Thread(const Thread& other) = delete;

    Thread(Thread&& other) : threadHandle(other.threadHandle) {
        other.threadHandle = 0;
    }

    inline bool Joinable() {
        return threadHandle != 0;
    }

    void Join() {
        if (threadHandle == 0)
            throw std::system_error(std::make_error_code(std::errc::no_such_process));
        pthread_join(threadHandle, nullptr);
        threadHandle = 0;
    }

    void Detach() {
        if (Joinable()) {
            throw std::system_error(std::make_error_code(std::errc::invalid_argument));
        }
        pthread_detach(threadHandle);
        threadHandle = 0;
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
        other.threadHandle = 0;
        return *this;
    }

    static unsigned HardwareConcurrency() {
        return pthread_getconcurrency();
    }

    template<class Rep, class Period>
    static void Sleep(const std::chrono::duration<Rep,Period>& duration) {
        auto sec = std::chrono::duration_cast<std::chrono::seconds>(duration);
        auto nano = std::chrono::duration_cast<std::chrono::nanoseconds>(duration - sec);
        struct timespec remaining = {
            static_cast<time_t>(sec.count()),
            static_cast<long>(nano.count())
        };
        while (nanosleep(&remaining, &remaining) == -1 && errno == EINTR) {}
    }

    static inline void Yield() {
        pthread_yield();
    }
};

class Mutex {
    pthread_mutex_t mutex;
public:
    inline Mutex() {
        pthread_mutex_init(&mutex, nullptr);
    }
    inline ~Mutex() {
        pthread_mutex_destroy(&mutex);
    }
    inline void Lock() {
        pthread_mutex_lock(&mutex);
    }
    inline void Unlock() {
        pthread_mutex_unlock(&mutex);
    }
    inline bool TryLock() {
        return 0 == pthread_mutex_trylock(&mutex);
    }
};

} // namespace AzCore