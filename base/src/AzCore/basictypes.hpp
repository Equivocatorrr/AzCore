/*
	File: basictypes.hpp
	Author: Philip Haynes
	Aliasing of basic numeric types to a shorter representation
*/
#ifndef AZCORE_BASICTYPES_HPP
#define AZCORE_BASICTYPES_HPP

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;
typedef char i8;
typedef short i16;
typedef int i32;
typedef long long i64;
typedef float f32;
typedef double f64;

typedef u16 char16;
typedef u32 char32;

typedef __float128 f128;
typedef unsigned __int128 u128;
typedef __int128 i128;

static_assert(sizeof(u8)  == 1);
static_assert(sizeof(u16) == 2);
static_assert(sizeof(u32) == 4);
static_assert(sizeof(u64) == 8);
static_assert(sizeof(i8)  == 1);
static_assert(sizeof(i16) == 2);
static_assert(sizeof(i32) == 4);
static_assert(sizeof(i64) == 8);

static_assert(sizeof(f32) == 4);
static_assert(sizeof(f64) == 8);

static_assert(sizeof(f128) == 16);
static_assert(sizeof(u128) == 16);
static_assert(sizeof(i128) == 16);

// Let's pretend we support compilers other than GCC for a moment.
#if 1
#if defined(__clang__)
	#define force_inline
#elif defined(__GNUG__)
	#define force_inline __attribute__((always_inline))
#elif defined(_MSC_VER)
	#define force_inline __forceinline
#endif
#else
	#define force_inline
#endif

namespace AzCore {}
namespace az = AzCore;

#ifdef NDEBUG
	#define Assert(condition, message) 
#else
	#include <stdio.h>
	#include <stdlib.h>
	#ifdef __unix
	#include <execinfo.h>
	inline void PrintBacktrace(FILE *file) {
		constexpr size_t stackSizeMax = 256;
		void *array[stackSizeMax];
		int size;

		size = backtrace(array, stackSizeMax);
		fprintf(file, "Backtrace:\n");
		backtrace_symbols_fd(array, size, fileno(file));
	}
	inline void PrintBacktrace() {
		PrintBacktrace(stderr);
	}
	#else
	inline void PrintBacktrace(FILE *file) {}
	inline void PrintBacktrace() {}
	#endif // __unix
	constexpr void _Assert(bool condition, const char *file, const char *line, const char *message) {
		if (!condition) {
			fprintf(stderr, "\033[96m%s\033[0m:\033[96m%s\033[0m Assert failed: \033[91m%s\033[0m\n", file, line, message);
			PrintBacktrace(stderr);
			abort();
		}
	}
	constexpr auto* _GetFileName(const char* const path) {
		const auto* startPosition = path;
		for (const auto* cur = path; *cur != '\0'; ++cur) {
			if (*cur == '\\' || *cur == '/') startPosition = cur+1;
		}
		return startPosition;
	}
	#define STRINGIFY_DAMMIT(x) #x
	#define STRINGIFY(x) STRINGIFY_DAMMIT(x)
	#define Assert(condition, message) _Assert((condition), _GetFileName(__FILE__), STRINGIFY(__LINE__), (message))
#endif

#endif // AZCORE_BASICTYPES_HPP
