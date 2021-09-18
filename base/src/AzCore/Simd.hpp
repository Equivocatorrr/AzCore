/*
	File: Simd.hpp
	Author: Philip Haynes
	Includes whichever version of Simd is desired based on number of lanes and instruction set.
*/

#ifndef AZCORE_SIMD_HPP
#define AZCORE_SIMD_HPP

#if __SSE2__
#include "SIMD/SimdSSE2.hpp"
#else
#error "Simd.hpp requires SSE2 at a minimum."
#endif

#if __AVX__
#include "SIMD/SimdAVX.hpp"
#endif

#endif // AZCORE_SIMD_HPP
