/*
    File: basictypes.hpp
    Author: Philip Haynes
    Aliasing of basic numeric types to a shorter representation
*/
#ifndef BASICTYPES_HPP
#define BASICTYPES_HPP

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
#ifdef __unix
typedef unsigned long u64;
#else
#ifdef __MINGW32__
typedef unsigned long long u64;
#endif
#endif
typedef char i8;
typedef short i16;
typedef int i32;
typedef long i64;
typedef float f32;
typedef double f64;

#endif
