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

#endif
