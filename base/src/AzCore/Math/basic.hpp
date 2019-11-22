/*
    File: basic.hpp
    Author: Philip Haynes
*/

#ifndef AZCORE_MATH_BASIC_HPP
#define AZCORE_MATH_BASIC_HPP

#include "../basictypes.hpp"
#include <math.h>

namespace AzCore {

const f64 halfpi64 = 1.5707963267948966;
const f64 pi64 = 3.1415926535897932;
const f64 tau64 = 6.2831853071795865;

const f32 halfpi = (f32)halfpi64;
const f32 pi = (f32)pi64;
const f32 tau = (f32)tau64;

enum Axis
{
    X = 0,
    Y = 1,
    Z = 2
};

enum Plane
{
    XY = 0,
    XZ = 1,
    XW = 2,
    YX = XY,
    YZ = 3,
    YW = 4,
    ZX = XZ,
    ZY = YZ,
    ZW = 5
};

/* math.h does this for us
#ifdef AZCORE_MATH_F32
    inline f32 sin(f32 a) { return sinf(a); }
    inline f32 cos(f32 a) { return cosf(a); }
    inline f32 tan(f32 a) { return tanf(a); }
    inline f32 asin(f32 a) { return asinf(a); }
    inline f32 acos(f32 a) { return acosf(a); }
    inline f32 atan(f32 a) { return atanf(a); }
    inline f32 atan2(f32 y, f32 x) { return atan2f(y, x); }
    inline f32 sqrt(f32 a) { return sqrtf(a); }
    inline f32 exp(f32 a) { return expf(a); }
    inline f32 log(f32 a) { return logf(a); }
    inline f32 log2(f32 a) { return log2f(a); }
    inline f32 log10(f32 a) { return log10f(a); }
    inline f32 pow(f32 a, f32 b) { return powf(a, b); }
    inline f32 floor(f32 a) { return floorf(a); }
    inline f32 round(f32 a) { return roundf(a); }
    inline f32 ceil(f32 a) { return ceilf(a); }
#endif
*/

} // namespace AzCore

template <typename T>
inline T square(const T &a)
{
    return a * a;
}

template <typename T>
inline T median(const T &a, const T &b, const T &c)
{
    if ((b >= a && a >= c) || (c >= a && a >= b))
        return a;
    if ((a >= b && b >= c) || (c >= b && b >= a))
        return b;
    if ((a >= c && c >= b) || (b >= c && c >= a))
        return c;
    return T();
}

template <typename T>
inline T min(const T &a, const T &b)
{
    return a > b ? b : a;
}

template <typename T>
inline T max(const T &a, const T &b)
{
    return a > b ? a : b;
}

template <typename T>
inline T clamp(const T &a, const T &b, const T &c)
{
    return median(a, b, c);
}

template <typename T>
inline T abs(const T &a)
{
    return a >= 0 ? a : -a;
}

template <typename T>
inline T sign(const T &a)
{
    return a >= 0 ? 1 : -1;
}

template <typename T, typename F>
inline T lerp(const T &a, const T &b, F factor)
{
    factor = clamp(factor, F(0.0), F(1.0));
    return a + (b - a) * factor;
}

template <typename T, typename F>
T decay(T a, T b, F halfLife, F timestep)
{
    F fac = exp(-timestep / halfLife);
    if (fac > F(1.0))
        fac = F(1.0);
    else if (fac < F(0.0))
        fac = F(0.0);
    return b * (F(1.0) - fac) + a * fac;
}

template <typename T>
inline T map(const T &in, const T &minFrom, const T &maxFrom, const T &minTo, const T &maxTo)
{
    return (in - minFrom) * (maxTo - minTo) / (maxFrom - minFrom) + minTo;
}

template <typename T>
inline T cubert(const T &a)
{
    return a >= T(0.0) ? pow(a, T(1.0 / 3.0)) : -pow(-a, T(1.0 / 3.0));
}

template <typename T>
inline T wrap(T a, T max)
{
    while (a > max)
    {
        a -= max;
    }
    while (a < 0)
    {
        a += max;
    }
    return a;
}

template <typename T>
T normalize(const T&);

#endif // AZCORE_MATH_BASIC_HPP
