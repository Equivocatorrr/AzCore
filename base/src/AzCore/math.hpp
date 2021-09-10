/*
	File: math.hpp
	Author: Philip Haynes
	Description: Common math routines and data types.
	Notes:
		- Vector math is right-handed.
		- Be aware of memory alignment when dealing with GPU memory.
*/
#ifndef AZCORE_MATH_HPP
#define AZCORE_MATH_HPP

// Some quick defines to change what gets implemented in math.cpp
// Comment these out to reduce compile size.
#define AZCORE_MATH_VEC2
#define AZCORE_MATH_VEC3
#define AZCORE_MATH_VEC4
#define AZCORE_MATH_VEC5
#define AZCORE_MATH_MAT2
#define AZCORE_MATH_MAT3
#define AZCORE_MATH_MAT4
#define AZCORE_MATH_MAT5
#define AZCORE_MATH_COMPLEX
#define AZCORE_MATH_QUATERNION
// AZCORE_MATH_EQUATIONS adds solvers for linear, quadratic, and cubic equations
#define AZCORE_MATH_EQUATIONS
#define AZCORE_MATH_F32
// #define AZCORE_MATH_F64

// Some are dependent on others
#if defined(AZCORE_MATH_MAT2) && !defined(AZCORE_MATH_VEC2)
	#define AZCORE_MATH_VEC2
#endif
#if defined(AZCORE_MATH_MAT3) && !defined(AZCORE_MATH_VEC3)
		#define AZCORE_MATH_VEC3
#endif
#if defined(AZCORE_MATH_MAT4) && !defined(AZCORE_MATH_VEC3)
	#define AZCORE_MATH_VEC3
#endif
#if defined(AZCORE_MATH_MAT4) && !defined(AZCORE_MATH_VEC4)
	#define AZCORE_MATH_VEC4
#endif
#if defined(AZCORE_MATH_QUATERNION) && !defined(AZCORE_MATH_VEC3)
	#define AZCORE_MATH_VEC3
#endif

#if !defined(AZCORE_MATH_F32) && !defined(AZCORE_MATH_F64)
	#error "math.hpp needs to have one or both of AZCORE_MATH_F32 and AZCORE_MATH_F64 defined"
#endif

#include "Math/basic.hpp"

#include "Math/Angle.hpp"

#include "Math/RandomNumberGenerator.hpp"

#ifdef AZCORE_MATH_VEC2
	#include "Math/vec2_t.hpp"
#endif // AZCORE_MATH_VEC2

#ifdef AZCORE_MATH_MAT2
	#include "Math/mat2_t.hpp"
#endif // AZCORE_MATH_MAT2

#ifdef AZCORE_MATH_VEC3
	#include "Math/vec3_t.hpp"
#endif // AZCORE_MATH_VEC3

#ifdef AZCORE_MATH_MAT3
	#include "Math/mat3_t.hpp"
#endif // AZCORE_MATH_MAT3

#ifdef AZCORE_MATH_VEC4
	#include "Math/vec4_t.hpp"
#endif // AZCORE_MATH_VEC4

#ifdef AZCORE_MATH_MAT4
	#include "Math/mat4_t.hpp"
#endif // AZCORE_MATH_MAT4

#ifdef AZCORE_MATH_VEC5
	#include "Math/vec5_t.hpp"
#endif // AZCORE_MATH_VEC5

#ifdef AZCORE_MATH_MAT5
	#include "Math/mat5_t.hpp"
#endif // AZCORE_MATH_MAT5

#ifdef AZCORE_MATH_COMPLEX
	#include "Math/complex_t.hpp"
#endif // AZCORE_MATH_COMPLEX

#ifdef AZCORE_MATH_QUATERNION
	#include "Math/quat_t.hpp"
#endif // AZCORE_MATH_QUATERNION

#ifdef AZCORE_MATH_EQUATIONS
	#include "Math/Equations.hpp"
#endif // AZCORE_MATH_EQUATIONS

template <typename T>
inline T normalize(T a)
{
	return a / abs(a);
}

// Typedefs for nice naming conventions

namespace AzCore {

#ifdef AZCORE_MATH_VEC2
	#ifdef AZCORE_MATH_F32
		typedef vec2_t<f32> vec2;
	#endif
	#ifdef AZCORE_MATH_F64
		typedef vec2_t<f64> vec2d;
	#endif
	typedef vec2_t<i32> vec2i;
#endif
#ifdef AZCORE_MATH_VEC3
	#ifdef AZCORE_MATH_F32
		typedef vec3_t<f32> vec3;
	#endif
	#ifdef AZCORE_MATH_F64
		typedef vec3_t<f64> vec3d;
	#endif
	typedef vec3_t<i32> vec3i;
#endif
#ifdef AZCORE_MATH_VEC4
	#ifdef AZCORE_MATH_F32
		typedef vec4_t<f32> vec4;
	#endif
	#ifdef AZCORE_MATH_F64
		typedef vec4_t<f64> vec4d;
	#endif
	typedef vec4_t<i32> vec4i;
#endif
#ifdef AZCORE_MATH_VEC5
	#ifdef AZCORE_MATH_F32
		typedef vec5_t<f32> vec5;
	#endif
	#ifdef AZCORE_MATH_F64
		typedef vec5_t<f64> vec5d;
	#endif
	typedef vec5_t<i32> vec5i;
#endif
#ifdef AZCORE_MATH_MAT2
	#ifdef AZCORE_MATH_F32
		typedef mat2_t<f32> mat2;
	#endif
	#ifdef AZCORE_MATH_F64
		typedef mat2_t<f64> mat2d;
	#endif
#endif
#ifdef AZCORE_MATH_MAT3
	#ifdef AZCORE_MATH_F32
		typedef mat3_t<f32> mat3;
	#endif
	#ifdef AZCORE_MATH_F64
		typedef mat3_t<f64> mat3d;
	#endif
#endif
#ifdef AZCORE_MATH_MAT4
	#ifdef AZCORE_MATH_F32
		typedef mat4_t<f32> mat4;
	#endif
	#ifdef AZCORE_MATH_F64
		typedef mat4_t<f64> mat4d;
	#endif
#endif
#ifdef AZCORE_MATH_MAT5
	#ifdef AZCORE_MATH_F32
		typedef mat5_t<f32> mat5;
	#endif
	#ifdef AZCORE_MATH_F64
		typedef mat5_t<f64> mat5d;
	#endif
#endif
#ifdef AZCORE_MATH_COMPLEX
	#ifdef AZCORE_MATH_F32
		typedef complex_t<f32> complex;
	#endif
	#ifdef AZCORE_MATH_F64
		typedef complex_t<f64> complexd;
	#endif
#endif
#ifdef AZCORE_MATH_QUATERNION
	#ifdef AZCORE_MATH_F32
		typedef quat_t<f32> quat;
	#endif
	#ifdef AZCORE_MATH_F64
		typedef quat_t<f64> quatd;
	#endif
#endif

} // namespace AzCore

#endif
