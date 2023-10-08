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
typedef signed char i8;
typedef short i16;
typedef int i32;
typedef long long i64;
typedef float f32;
typedef double f64;

typedef u16 char16;
typedef u32 char32;

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

namespace AzCore {
	static constexpr i32 indexIndicatingRaw = (i32)0xFFFFFFFF; // Because MSVC is stupid
}

#if 1
#if defined(__clang__)
	#define f128 static_assert(false && "f128 is not supported in this compiler");
	#define u128 static_assert(false && "u128 is not supported in this compiler");
	#define i128 static_assert(false && "i128 is not supported in this compiler");
	#define force_inline(...) inline __VA_ARGS__
	#define AZCORE_COMPILER_SUPPORTS_128BIT_TYPES 0
#elif defined(__GNUG__)
	typedef __float128 f128;
	typedef unsigned __int128 u128;
	typedef __int128 i128;
	static_assert(sizeof(f128) == 16);
	static_assert(sizeof(u128) == 16);
	static_assert(sizeof(i128) == 16);
	#define force_inline(...) inline __VA_ARGS__ __attribute__((always_inline))
	#define AZCORE_COMPILER_SUPPORTS_128BIT_TYPES 1
#elif defined(_MSC_VER)
	#define f128 static_assert(false && "f128 is not supported in this compiler");
	#define u128 static_assert(false && "u128 is not supported in this compiler");
	#define i128 static_assert(false && "i128 is not supported in this compiler");
	#define force_inline(...) __forceinline __VA_ARGS__
	// #define force_inline(...) inline __VA_ARGS__
	#define AZCORE_COMPILER_SUPPORTS_128BIT_TYPES 0
#endif
#else
	#define force_inline(...) inline __VA_ARGS__
	#define AZCORE_COMPILER_SUPPORTS_128BIT_TYPES 0
#endif

#if defined(_MSC_VER)
#define AZ_MSVC_ONLY(a) a
#else
#define AZ_MSVC_ONLY(a)
#endif

// NOTE: Because of how mingw64 works, basictypes.hpp must be included before any system headers
#ifdef WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define WINVER 0x0A00
#define _WIN32_WINNT 0x0A00
#endif // WIN32

namespace AzCore {}
namespace az = AzCore;

#endif // AZCORE_BASICTYPES_HPP
