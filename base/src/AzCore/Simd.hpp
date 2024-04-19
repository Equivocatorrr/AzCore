/*
	File: Simd.hpp
	Author: Philip Haynes
	Includes whichever version of Simd is desired based on number of lanes and instruction set.
*/

#ifndef AZCORE_SIMD_HPP
#define AZCORE_SIMD_HPP

#ifdef _MSC_VER
	#if __AVX__
		#ifndef __SSE4_2__
			#define __SSE4_2__ 1
		#endif
	#endif
	#if __SSE4_2__
		#ifndef __SSE4_1__
			#define __SSE4_1__ 1
		#endif
	#endif
	#if __SSE4_1__
		#ifndef __SSSE3__
			#define __SSSE3__ 1
		#endif
	#endif
	#if __SSSE3__
		#ifndef __SSE3__
			#define __SSE3__ 1
		#endif
	#endif
	#if __SSE3__
		#ifndef __SSE2__
			#define __SSE2__ 1
		#endif
	#endif
	#if __SSE2__
		#ifndef __SSE__
			#define __SSE__ 1
		#endif
	#endif
#endif

#if __SSE2__
#include "SIMD/SimdSSE2.hpp"
#else
#error "Simd.hpp requires SSE2 at a minimum."
#endif

#if __AVX__
#include "SIMD/SimdAVX.hpp"
#endif

#endif // AZCORE_SIMD_HPP
