/*
    File: math.hpp
    Author: Philip Haynes
    Description: Common math routines and data types.
    Notes:
        - Vector math is right-handed.
        - Be aware of memory alignment when dealing with GPU memory.
*/
#ifndef MATH_HPP
#define MATH_HPP

#include "basictypes.hpp"

#include <chrono>

using Nanoseconds = std::chrono::nanoseconds;
using Milliseconds = std::chrono::milliseconds;
using Clock = std::chrono::steady_clock;
using ClockTime = std::chrono::steady_clock::time_point;

#include <cmath>

const f64 halfpi64  = 1.5707963267948966;
const f64 pi64      = 3.1415926535897932;
const f64 tau64     = 6.2831853071795865;

const f32 halfpi    = (f32)halfpi64;
const f32 pi        = (f32)pi64;
const f32 tau       = (f32)tau64;

enum Axis {
    X=0, Y=1, Z=2
};

enum Plane {
    XY=0,   XZ=1,   XW=2,
    YX=XY,  YZ=3,   YW=4,
    ZX=XZ,  ZY=YZ,  ZW=5
};

inline f32 cos(const f32& a) { return cosf(a); }
inline f32 sin(const f32& a) { return sinf(a); }
inline f32 tan(const f32& a) { return tanf(a); }
inline f32 asin(const f32& a) { return asinf(a); }
inline f32 acos(const f32& a) { return acosf(a); }
inline f32 atan(const f32& a) { return atanf(a); }
inline f32 atan2(const f32& y, const f32& x) { return atan2f(y, x); }
inline f32 sqrt(const f32& a) { return sqrtf(a); }

/*  struct: RandomNumberGenerator
    Author: Philip Haynes
    Uses the JKISS generator by David Jones
    From http://www0.cs.ucl.ac.uk/staff/d.jones/GoodPracticeRNG.pdf */
struct RandomNumberGenerator {
    u32 x, y, z, c;
    RandomNumberGenerator(); // Automatically seeds itself based on time.
    u32 Generate();
    void Seed(u64 seed);
};

template<typename T>
inline T square(const T a) {
    return a*a;
}

template<typename T>
inline T median(T a, T b, T c) {
    if ((b >= a && a >= c)||(c >= a && a >= b))
        return a;
    if ((a >= b && b >= c)||(c >= b && b >= a))
        return b;
    if ((a >= c && c >= b)||(b >= c && c >= a))
        return c;
    return T();
}

template<typename T>
inline T min(T a, T b) {
	return a > b ? b : a;
}

template<typename T>
inline T max(T a, T b) {
	return a > b ? a : b;
}

template<typename T>
inline T clamp(T a, T b, T c) {
    return median(a, b, c);
}

template<typename T>
inline T sign(T a) {
    return a >= 0 ? 1 : -1;
}

// Finds the shortest distance from one angle to another.
f32 angleDiff(f32 from, f32 to);
f64 angleDiff(f64 from, f64 to);

template<typename T>
inline T angleDir(const T& from, const T& to) {
    return sign(angleDiff(from, to));
}

template<typename T>
inline T normalize(const T& a) {
    return a / length(a);
}

template<typename T>
struct vec2_t {
    union {
        struct {
            T x, y;
        };
        struct {
            T u, v;
        };
        struct {
            T data[2];
        };
    };

    vec2_t();
    vec2_t(T a);
    vec2_t(T a, T b);
    inline vec2_t<T> operator+(const vec2_t<T>& a) const { return vec2_t<T>(x+a.x, y+a.y); }
    inline vec2_t<T> operator-(const vec2_t<T>& a) const { return vec2_t<T>(x-a.x, y-a.y); }
    inline vec2_t<T> operator-() const { return vec2_t<T>(-x, -y); }
    inline vec2_t<T> operator*(const vec2_t<T>& a) const { return vec2_t<T>(x*a.x, y*a.y); }
    inline vec2_t<T> operator/(const vec2_t<T>& a) const { return vec2_t<T>(x/a.x, y/a.y); }
    inline vec2_t<T> operator*(const T& a) const { return vec2_t<T>(x*a, y*a); }
    inline vec2_t<T> operator/(const T& a) const { return vec2_t<T>(x/a, y/a); }
    inline T& operator[](const u32& i) { return data[i]; }
    vec2_t<T> operator+=(const vec2_t<T>& a);
    vec2_t<T> operator-=(const vec2_t<T>& a);
    vec2_t<T> operator/=(const vec2_t<T>& a);
    vec2_t<T> operator/=(const T& a);
    vec2_t<T> operator*=(const vec2_t<T>& a);
    vec2_t<T> operator*=(const T& a);
};

template<typename T>
inline T dot(const vec2_t<T>& a, const vec2_t<T>& b) {
    return a.x*b.x + a.y*b.y;
}

template<typename T>
inline T length(const vec2_t<T>& a) {
    return sqrt(a.x*a.x + a.y*a.y);
}

template<typename T>
struct mat2_t {
    union {
        struct {
            T x1, y1,
              x2, y2;
        } h;
        struct {
            T x1, x2,
              y1, y2;
        } v;
        struct {
            T data[4];
        };
    };
    mat2_t();
    mat2_t(T a);
    mat2_t(T a, T b, T c, T d);
    mat2_t(vec2_t<T> a, vec2_t<T> b, bool rowMajor=true);
    mat2_t(T d[4]);
    inline vec2_t<T> Row1() const { return vec2_t<T>(h.x1, h.y1); }
    inline vec2_t<T> Row2() const { return vec2_t<T>(h.x2, h.y2); }
    inline vec2_t<T> Col1() const { return vec2_t<T>(v.x1, v.y1); }
    inline vec2_t<T> Col2() const { return vec2_t<T>(v.x2, v.y2); }
    static mat2_t<T> Rotation(T angle);
    inline mat2_t<T> Rotate(const T& angle) const {
        return mat2_t<T>::Rotation(angle) * (*this);
    }
    static mat2_t<T> Skewer(vec2_t<T> amount);
    inline mat2_t<T> Skew(const vec2_t<T>& amount) const {
        return mat2_t<T>::Skewer(amount) * (*this);
    }
    static mat2_t<T> Scaler(vec2_t<T> scale);
    inline mat2_t<T> Scale(const vec2_t<T>& scale) const {
        return mat2_t<T>::Scaler(scale) * (*this);
    }
    inline mat2_t<T> Transpose() const {
        return mat2_t<T>(v.x1, v.y1, v.x2, v.y2);
    }
    inline mat2_t<T> operator+(const mat2_t<T>& a) const {
        return mat2_t<T>(
            h.x1 + a.h.x1, h.y1 + a.h.y1,
            h.x2 + a.h.x2, h.y2 + a.h.y2
        );
    }
    inline mat2_t<T> operator*(const mat2_t<T>& a) const {
        return mat2_t<T>(
            h.x1 * a.v.x1 + h.y1 * a.v.y1, h.x1 * a.v.x2 + h.y1 * a.v.y2,
            h.x2 * a.v.x1 + h.y2 * a.v.y1, h.x2 * a.v.x2 + h.y2 * a.v.y2
        );
    }
    inline vec2_t<T> operator*(const vec2_t<T>& a) const {
        return vec2_t<T>(
            h.x1*a.x + h.y1*a.y,
            h.x2*a.x + h.y2*a.y
        );
    }
    inline mat2_t<T> operator*(const T& a) const {
        return mat2_t<T>(
            h.x1*a, h.y1*a,
            h.x2*a, h.y2*a
        );
    }
    inline mat2_t<T> operator/(const vec2_t<T>& a) const {
        return mat2_t<T>(
            h.x1/a.x, h.y1/a.y,
            h.x2/a.x, h.y2/a.y
        );
    }
};

template<typename T>
struct vec3_t {
    union {
        struct {
            T x, y, z;
        };
        struct {
            T r, g, b;
        };
        struct {
            T data[3];
        };
    };

    vec3_t();
    vec3_t(T v);
    vec3_t(T v1, T v2, T v3);
    inline vec3_t<T> operator+(const vec3_t<T>& v) const { return vec3_t<T>(x+v.x, y+v.y, z+v.z); }
    inline vec3_t<T> operator-(const vec3_t<T>& v) const { return vec3_t<T>(x-v.x, y-v.y, z-v.z); }
    inline vec3_t<T> operator-() const { return vec3_t<T>(-x, -y, -z); }
    inline vec3_t<T> operator*(const vec3_t<T>& v) const { return vec3_t<T>(x*v.x, y*v.y, z*v.z); }
    inline vec3_t<T> operator/(const vec3_t<T>& v) const { return vec3_t<T>(x/v.x, y/v.y, z/v.z); }
    inline vec3_t<T> operator*(const T& v) const { return vec3_t<T>(x*v, y*v, z*v); }
    inline vec3_t<T> operator/(const T& v) const { return vec3_t<T>(x/v, y/v, z/v); }
    inline T& operator[](const u32& i) { return data[i]; }
    vec3_t<T> operator+=(const vec3_t<T>& v);
    vec3_t<T> operator-=(const vec3_t<T>& v);
    vec3_t<T> operator/=(const vec3_t<T>& v);
    vec3_t<T> operator/=(const T& v);
    vec3_t<T> operator*=(const vec3_t<T>& v);
    vec3_t<T> operator*=(const T& v);
};

template<typename T>
inline vec3_t<T> cross(const vec3_t<T>& a, const vec3_t<T>& b) {
    return vec3_t<T>(
        a.y*b.z - a.z*b.y,
        a.z*b.x - a.x*b.z,
        a.x*b.y - a.y*b.x
    );
}

template<typename T>
inline T dot(const vec3_t<T>& a, const vec3_t<T>& b) {
    return a.x*b.x + a.y*b.y + a.z*b.z;
}

template<typename T>
inline T length(const vec3_t<T>& a) {
    return sqrt(a.x*a.x + a.y*a.y + a.z*a.z);
}

template<typename T>
struct mat3_t {
    union {
        struct {
            T x1, y1, z1,
              x2, y2, z2,
              x3, y3, z3;
        } h;
        struct {
            T x1, x2, x3,
              y1, y2, y3,
              z1, z2, z3;
        } v;
        struct {
            T data[9];
        };
    };
    mat3_t();
    mat3_t(T a);
    mat3_t(T x1, T y1, T z1, T x2, T y2, T z2, T x3, T y3, T z3);
    mat3_t(vec3_t<T> a, vec3_t<T> b, vec3_t<T> c, bool rowMajor=true);
    mat3_t(T d[9]);
    inline vec3_t<T> Row1() const { return vec3_t<T>(h.x1, h.y1, h.z1); }
    inline vec3_t<T> Row2() const { return vec3_t<T>(h.x2, h.y2, h.z2); }
    inline vec3_t<T> Row3() const { return vec3_t<T>(h.x3, h.y3, h.z3); }
    inline vec3_t<T> Col1() const { return vec3_t<T>(v.x1, v.y1, v.z1); }
    inline vec3_t<T> Col2() const { return vec3_t<T>(v.x2, v.y2, v.z2); }
    inline vec3_t<T> Col3() const { return vec3_t<T>(v.x3, v.y3, v.z3); }
    // Only useful for rotations about aligned axes, such as {1, 0, 0}
    static mat3_t<T> RotationBasic(T angle, Axis axis);
    inline mat3_t<T> RotateBasic(const T& angle, const Axis& axis) const {
        return mat3_t<T>::RotationBasic(angle, axis) * (*this);
    }
    // Useful for arbitrary axes
    static mat3_t<T> Rotation(T angle, vec3_t<T> axis);
    inline mat3_t<T> Rotate(const T& angle, const vec3_t<T> axis) const {
        return mat3_t<T>::Rotation(angle, axis) * (*this);
    }
    static mat3_t<T> Scaler(vec3_t<T> scale);
    inline mat3_t<T> Scale(const vec3_t<T>& scale) const {
        return mat3_t<T>::Scaler(scale) * (*this);
    }
    inline mat3_t<T> Transpose() const {
        return mat3_t<T>(v.x1, v.y1, v.z1, v.x2, v.y2, v.z2, v.x3, v.y3, v.z3);
    }
    inline mat3_t<T> operator+(const mat3_t<T>& a) const {
        return mat3_t<T>(
            h.x1 + a.h.x1, h.y1 + a.h.y1, h.z1 + a.h.z1,
            h.x2 + a.h.x2, h.y2 + a.h.y2, h.z2 + a.h.z2,
            h.x3 + a.h.x3, h.y3 + a.h.y3, h.z3 + a.h.z3
        );
    }
    inline mat3_t<T> operator*(const mat3_t<T>& a) const {
        return mat3_t<T>(
            h.x1 * a.v.x1 + h.y1 * a.v.y1 + h.z1 * a.v.z1,
            h.x1 * a.v.x2 + h.y1 * a.v.y2 + h.z1 * a.v.z2,
            h.x1 * a.v.x3 + h.y1 * a.v.y3 + h.z1 * a.v.z3,
            h.x2 * a.v.x1 + h.y2 * a.v.y1 + h.z2 * a.v.z1,
            h.x2 * a.v.x2 + h.y2 * a.v.y2 + h.z2 * a.v.z2,
            h.x2 * a.v.x3 + h.y2 * a.v.y3 + h.z2 * a.v.z3,
            h.x3 * a.v.x1 + h.y3 * a.v.y1 + h.z3 * a.v.z1,
            h.x3 * a.v.x2 + h.y3 * a.v.y2 + h.z3 * a.v.z2,
            h.x3 * a.v.x3 + h.y3 * a.v.y3 + h.z3 * a.v.z3
        );
    }
    inline vec3_t<T> operator*(const vec3_t<T>& a) const {
        return vec3_t<T>(
            h.x1*a.x + h.y1*a.y + h.z1*a.z,
            h.x2*a.x + h.y2*a.y + h.z2*a.z,
            h.x3*a.x + h.y3*a.y + h.z3*a.z
        );
    }
    inline mat3_t<T> operator*(const T& a) const {
        return mat3_t<T>(
            h.x1*a, h.y1*a, h.z1*a,
            h.x2*a, h.y2*a, h.z2*a,
            h.x3*a, h.y3*a, h.z3*a
        );
    }
    inline mat3_t<T> operator/(const vec3_t<T>& a) const {
        return mat3_t<T>(
            h.x1/a.x + h.y1/a.y + h.z1/a.z,
            h.x2/a.x + h.y2/a.y + h.z2/a.z,
            h.x3/a.x + h.y3/a.y + h.z3/a.z
        );
    }
    inline mat3_t<T> operator/(const T& a) const {
        return mat3_t<T>(
            h.x1/a + h.y1/a + h.z1/a,
            h.x2/a + h.y2/a + h.z2/a,
            h.x3/a + h.y3/a + h.z3/a
        );
    }
};

template<typename T>
struct vec4_t {
    union {
        struct {
            T x, y, z, w;
        };
        struct {
            vec3_t<T> xyz;
        };
        struct {
            T r, g, b, a;
        };
        struct {
            vec3_t<T> rgb;
        };
        struct {
            T data[4];
        };
    };

    vec4_t();
    vec4_t(T v);
    vec4_t(T v1, T v2, T v3, T v4);
    inline vec4_t<T> operator+(const vec4_t<T>& v) const { return vec4_t<T>(x+v.x, y+v.y, z+v.z, w+v.w); }
    inline vec4_t<T> operator-(const vec4_t<T>& v) const { return vec4_t<T>(x-v.x, y-v.y, z-v.z, w-v.w); }
    inline vec4_t<T> operator-() const { return vec4_t<T>(-x, -y, -z, -w); }
    inline vec4_t<T> operator*(const vec4_t<T>& v) const { return vec4_t<T>(x*v.x, y*v.y, z*v.z, w*v.w); }
    inline vec4_t<T> operator/(const vec4_t<T>& v) const { return vec4_t<T>(x/v.x, y/v.y, z/v.z, w/v.w); }
    inline vec4_t<T> operator*(const T& v) const { return vec4_t<T>(x*v, y*v, z*v, w*v); }
    inline vec4_t<T> operator/(const T& v) const { return vec4_t<T>(x/v, y/v, z/v, w/v); }
    inline T& operator[](const u32& i) { return data[i]; }
    vec4_t<T> operator+=(const vec4_t<T>& v);
    vec4_t<T> operator-=(const vec4_t<T>& v);
    vec4_t<T> operator/=(const vec4_t<T>& v);
    vec4_t<T> operator/=(const T& v);
    vec4_t<T> operator*=(const vec4_t<T>& v);
    vec4_t<T> operator*=(const T& v);
};

template<typename T>
inline T dot(const vec4_t<T>& a, const vec4_t<T>& b) {
    return a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w;
}

template<typename T>
inline T length(const vec4_t<T>& a) {
    return sqrt(a.x*a.x + a.y*a.y + a.z*a.z + a.w*a.w);
}

template<typename T>
struct mat4_t {
    union {
        struct {
            T x1, y1, z1, w1,
              x2, y2, z2, w2,
              x3, y3, z3, w3,
              x4, y4, z4, w4;
        } h;
        struct {
            T x1, x2, x3, x4,
              y1, y2, y3, y4,
              z1, z2, z3, z4,
              w1, w2, w3, w4;
        } v;
        struct {
            T data[16];
        };
    };
    mat4_t();
    mat4_t(T a);
    mat4_t(T x1, T y1, T z1, T w1,
           T x2, T y2, T z2, T w2,
           T x3, T y3, T z3, T w3,
           T x4, T y4, T z4, T w4);
    mat4_t(vec4_t<T> a, vec4_t<T> b, vec4_t<T> c, vec4_t<T> d, bool rowMajor=true);
    mat4_t(T d[16]);
    inline vec4_t<T> Row1() const { return vec4_t<T>(h.x1, h.y1, h.z1, h.w1); }
    inline vec4_t<T> Row2() const { return vec4_t<T>(h.x2, h.y2, h.z2, h.w2); }
    inline vec4_t<T> Row3() const { return vec4_t<T>(h.x3, h.y3, h.z3, h.w3); }
    inline vec4_t<T> Row4() const { return vec4_t<T>(h.x4, h.y4, h.z4, h.w4); }
    inline vec4_t<T> Col1() const { return vec4_t<T>(v.x1, v.y1, v.z1, v.w1); }
    inline vec4_t<T> Col2() const { return vec4_t<T>(v.x2, v.y2, v.z2, v.w2); }
    inline vec4_t<T> Col3() const { return vec4_t<T>(v.x3, v.y3, v.z3, v.w3); }
    inline vec4_t<T> Col4() const { return vec4_t<T>(v.x4, v.y4, v.z4, v.w4); }
    // Only useful for rotations about aligned planes, such as {{1, 0, 0, 0}, {0, 0, 0, 1}}
    // Note: The planes stay fixed in place and everything else rotates around them.
    static mat4_t<T> RotationBasic(T angle, Plane plane);
    // For using 3D-axis rotations
    static mat4_t<T> RotationBasic(T angle, Axis axis);
    inline mat4_t<T> RotateBasic(const T& angle, const Plane& plane) const {
        return mat4_t<T>::RotationBasic(angle, plane) * (*this);
    }
    inline mat4_t<T> RotateBasic(const T& angle, const Axis& axis) const {
        return mat4_t<T>::RotationBasic(angle, axis) * (*this);
    }
    // Useful for arbitrary 3D-axes
    static mat4_t<T> Rotation(T angle, vec3_t<T> axis);
    inline mat4_t<T> Rotate(const T& angle, const vec3_t<T> axis) const {
        return mat4_t<T>::Rotation(angle, axis) * (*this);
    }
    static mat4_t<T> Scaler(vec4_t<T> scale);
    inline mat4_t<T> Scale(const vec4_t<T>& scale) const {
        return mat4_t<T>::Scaler(scale) * (*this);
    }
    inline mat4_t<T> Transpose() const {
        return mat4_t<T>(v.x1, v.y1, v.z1, v.w1,
                         v.x2, v.y2, v.z2, v.w2,
                         v.x3, v.y3, v.z3, v.w3,
                         v.x4, v.y4, v.z4, v.w4);
    }
    inline mat4_t<T> operator+(const mat4_t<T>& a) const {
        return mat4_t<T>(
            h.x1 + a.h.x1, h.y1 + a.h.y1, h.z1 + a.h.z1, h.w1 + a.h.w1,
            h.x2 + a.h.x2, h.y2 + a.h.y2, h.z2 + a.h.z2, h.w2 + a.h.w2,
            h.x3 + a.h.x3, h.y3 + a.h.y3, h.z3 + a.h.z3, h.w3 + a.h.w3,
            h.x4 + a.h.x4, h.y4 + a.h.y4, h.z4 + a.h.z4, h.w4 + a.h.w4
        );
    }
    inline mat4_t<T> operator*(const mat4_t<T>& a) const {
        return mat4_t<T>(
            h.x1 * a.v.x1 + h.y1 * a.v.y1 + h.z1 * a.v.z1 + h.w1 * a.v.w1,
            h.x1 * a.v.x2 + h.y1 * a.v.y2 + h.z1 * a.v.z2 + h.w1 * a.v.w2,
            h.x1 * a.v.x3 + h.y1 * a.v.y3 + h.z1 * a.v.z3 + h.w1 * a.v.w3,
            h.x1 * a.v.x4 + h.y1 * a.v.y4 + h.z1 * a.v.z4 + h.w1 * a.v.w4,
            h.x2 * a.v.x1 + h.y2 * a.v.y1 + h.z2 * a.v.z1 + h.w2 * a.v.w1,
            h.x2 * a.v.x2 + h.y2 * a.v.y2 + h.z2 * a.v.z2 + h.w2 * a.v.w2,
            h.x2 * a.v.x3 + h.y2 * a.v.y3 + h.z2 * a.v.z3 + h.w2 * a.v.w3,
            h.x2 * a.v.x4 + h.y2 * a.v.y4 + h.z2 * a.v.z4 + h.w2 * a.v.w4,
            h.x3 * a.v.x1 + h.y3 * a.v.y1 + h.z3 * a.v.z1 + h.w3 * a.v.w1,
            h.x3 * a.v.x2 + h.y3 * a.v.y2 + h.z3 * a.v.z2 + h.w3 * a.v.w2,
            h.x3 * a.v.x3 + h.y3 * a.v.y3 + h.z3 * a.v.z3 + h.w3 * a.v.w3,
            h.x3 * a.v.x4 + h.y3 * a.v.y4 + h.z3 * a.v.z4 + h.w3 * a.v.w4,
            h.x4 * a.v.x1 + h.y4 * a.v.y1 + h.z4 * a.v.z1 + h.w4 * a.v.w1,
            h.x4 * a.v.x2 + h.y4 * a.v.y2 + h.z4 * a.v.z2 + h.w4 * a.v.w2,
            h.x4 * a.v.x3 + h.y4 * a.v.y3 + h.z4 * a.v.z3 + h.w4 * a.v.w3,
            h.x4 * a.v.x4 + h.y4 * a.v.y4 + h.z4 * a.v.z4 + h.w4 * a.v.w4
        );
    }
    inline vec4_t<T> operator*(const vec4_t<T>& a) const {
        return vec4_t<T>(
            h.x1*a.x + h.y1*a.y + h.z1*a.z + h.w1*a.w,
            h.x2*a.x + h.y2*a.y + h.z2*a.z + h.w2*a.w,
            h.x3*a.x + h.y3*a.y + h.z3*a.z + h.w3*a.w,
            h.x4*a.x + h.y4*a.y + h.z4*a.z + h.w4*a.w
        );
    }
    inline mat4_t<T> operator*(const T& a) const {
        return mat4_t<T>(
            h.x1*a, h.y1*a, h.z1*a, h.w1*a,
            h.x2*a, h.y2*a, h.z2*a, h.w2*a,
            h.x3*a, h.y3*a, h.z3*a, h.w3*a,
            h.x4*a, h.y4*a, h.z4*a, h.w4*a
        );
    }
    inline mat4_t<T> operator/(const vec4_t<T>& a) const {
        return mat4_t<T>(
            h.x1/a.x + h.y1/a.y + h.z1/a.z + h.w1/a.w,
            h.x2/a.x + h.y2/a.y + h.z2/a.z + h.w2/a.w,
            h.x3/a.x + h.y3/a.y + h.z3/a.z + h.w3/a.w,
            h.x4/a.x + h.y4/a.y + h.z4/a.z + h.w4/a.w
        );
    }
    inline mat4_t<T> operator/(const T& a) const {
        return mat4_t<T>(
            h.x1/a + h.y1/a + h.z1/a + h.w1/a,
            h.x2/a + h.y2/a + h.z2/a + h.w2/a,
            h.x3/a + h.y3/a + h.z3/a + h.w3/a,
            h.x4/a + h.y4/a + h.z4/a + h.w4/a
        );
    }
};

template<typename T>
struct complex_t {
    union {
        struct {
            T real, imag;
        };
        struct {
            vec2_t<T> vector;
        };
        struct {
            T x, y;
        };
    };

    complex_t();
    complex_t(T a);
    complex_t(T a, T b);
    complex_t(vec2_t<T> vec);
    complex_t(T d[2]);

    inline complex_t<T> operator*(const complex_t<T>& a) const {
        return complex_t<T>(real*a.real - imag*a.imag, real*a.imag + imag*a.real);
    }
    inline complex_t<T> operator*(const T& a) const {
        return complex_t<T>(real*a, imag*a);
    }
    inline complex_t<T> operator/(const complex_t<T>& a) const {
        return (*this) * a.Reciprocal();
    }
    inline complex_t<T> operator/(const T& a) const {
        return complex_t<T>(real/a, imag/a);
    }
    inline complex_t<T> operator+(const complex_t<T>& a) const {
        return complex_t<T>(real + a.real, imag + a.imag);
    }
    inline complex_t<T> operator+(const T& a) const {
        return complex_t<T>(real + a, imag);
    }
    inline complex_t<T> operator-(const complex_t<T>& a) const {
        return complex_t<T>(real - a.real, imag - a.imag);
    }
    inline complex_t<T> operator-(const T& a) const {
        return complex_t<T>(real - a, imag);
    }
    inline complex_t<T> Conjugate() const {
        return complex_t<T>(real, -imag);
    }
    inline complex_t<T> Reciprocal() const {
        return Conjugate() / dot(vector, vector);
    }
};

template<typename T>
inline T length(const complex_t<T>& a) {
    return sqrt(a.x*a.x + a.y*a.y);
}

template<typename T>
struct quat_t {
    union {
        struct {
            T w, x, y, z;
        };
        struct {
            T scalar;
            vec3_t<T> vector;
        };
        struct {
            vec4_t<T> wxyz;
        };
        struct {
            T data[4];
        };
    };

    quat_t();
    quat_t(T a);
    quat_t(T a, vec3_t<T> v);
    quat_t(vec4_t<T> v);
    quat_t(T a, T b, T c, T d);
    quat_t(T d[4]);

    inline quat_t<T> operator*(const quat_t<T>& a) const {
        return quat_t<T>(
            w*a.w - x*a.x - y*a.y - z*a.z,
            w*a.x + x*a.w + y*a.z - z*a.y,
            w*a.y - x*a.z + y*a.w + z*a.x,
            w*a.z + x*a.y - y*a.x + z*a.w
        );
    }
    inline quat_t<T> operator*(const T& a) const {
        return quat_t<T>(wxyz*a);
    }
    inline quat_t<T> operator/(const quat_t<T>& a) const {
        return (*this) * a.Reciprocal();
    }
    inline quat_t<T> operator/(const T& a) const {
        return quat_t<T>(wxyz/a);
    }
    inline quat_t<T> operator-(const quat_t<T>& a) const {
        return quat_t<T>(wxyz-a.wxyz);
    }
    inline quat_t<T> operator+(const quat_t<T>& a) const {
        return quat_t<T>(wxyz+a.wxyz);
    }
    inline quat_t<T> Conjugate() const {
        return quat_t<T>(scalar, -vector);
    }
    inline T Norm() const {
        return length(wxyz);
    }
    inline quat_t<T> Reciprocal() const {
        return Conjugate() / dot(wxyz, wxyz); // For unit quaternions just use Conjugate()
    }
    // Make a rotation quaternion
    static inline quat_t<T> Rotation(const T& angle, const vec3_t<T>& axis) {
        return quat_t<T>(cos(angle/2.0), normalize(axis) * sin(angle/2.0));
    }
    // A one-off rotation of a point
    static vec3_t<T> RotatePoint(vec3_t<T> point, T angle, vec3_t<T> axis);
    // Using this quaternion for a one-off rotation of a point
    vec3_t<T> RotatePoint(vec3_t<T> point) const;
    // Rotating this quaternion about an axis
    quat_t<T> Rotate(T angle, vec3_t<T> axis) const;
    // Rotate this quaternion by using a specified rotation quaternion
    quat_t<T> Rotate(quat_t<T> rotation) const;
    // Convert this rotation quaternion into a matrix
    mat3_t<T> ToMatrix() const;
};

template<typename T>
inline quat_t<T> normalize(const quat_t<T>& a) {
    return a / a.Norm();
}

template<typename T>
quat_t<T> slerp(quat_t<T> a, quat_t<T> b, T factor);

typedef vec2_t<f32> vec2;
typedef vec2_t<f64> vec2d;
typedef vec2_t<i32> vec2i;

typedef vec3_t<f32> vec3;
typedef vec3_t<f64> vec3d;
typedef vec3_t<i32> vec3i;

typedef vec4_t<f32> vec4;
typedef vec4_t<f64> vec4d;
typedef vec4_t<i32> vec4i;

typedef mat2_t<f32> mat2;
typedef mat2_t<f64> mat2d;

typedef mat3_t<f32> mat3;
typedef mat3_t<f64> mat3d;

typedef mat4_t<f32> mat4;
typedef mat4_t<f64> mat4d;

typedef complex_t<f32> complex;
typedef complex_t<f64> complexd;

typedef quat_t<f32> quat;
typedef quat_t<f64> quatd;

#endif
