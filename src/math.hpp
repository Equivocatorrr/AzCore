/*
    File: math.hpp
    Author: Philip Haynes
    Description: Common math routines and data types.
*/
#ifndef MATH_HPP
#define MATH_HPP

#include "basictypes.hpp"

#include <cmath>

const f32 halfpi = 1.5707963267948966;
const f32 pi = 3.1415926535897932;
const f32 tau = 6.2831853071795865;

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

    vec2_t<T>() : x(0) , y(0) {}
    vec2_t<T>(T a) : x(a) , y (a) {}
    vec2_t<T>(T a, T b) : x(a) , y(b) {}
    inline vec2_t<T> operator+(const vec2_t<T>& a) const { return vec2_t<T>(x+a.x, y+a.y); }
    inline vec2_t<T> operator-(const vec2_t<T>& a) const { return vec2_t<T>(x-a.x, y-a.y); }
    inline vec2_t<T> operator-() const { return vec2_t<T>(-x, -y); }
    inline vec2_t<T> operator*(const vec2_t<T>& a) const { return vec2_t<T>(x*a.x, y*a.y); }
    inline vec2_t<T> operator/(const vec2_t<T>& a) const { return vec2_t<T>(x/a.x, y/a.y); }
    inline vec2_t<T> operator+=(const vec2_t<T>& a) {
        x += a.x;
        y += a.y;
        return *this;
    }
    inline vec2_t<T> operator-=(const vec2_t<T>& a) {
        x -= a.x;
        y -= a.y;
        return *this;
    }
    inline vec2_t<T> operator/=(const vec2_t<T>& a) {
        x /= a.x;
        y /= a.y;
        return *this;
    }
    inline vec2_t<T> operator/=(const T& a) {
        x /= a;
        y /= a;
        return *this;
    }
    inline vec2_t<T> operator*=(const vec2_t<T>& a) {
        x *= a.x;
        y *= a.y;
        return *this;
    }
    inline vec2_t<T> operator*=(const T& a) {
        x *= a;
        y *= a;
        return *this;
    }
    inline vec2_t<T> operator*(const T& a) const { return vec2_t<T>(x*a, y*a); }
    inline vec2_t<T> operator/(const T& a) const { return vec2_t<T>(x/a, y/a); }
    inline T& operator[](const unsigned i) { return data[i]; }
};

template<typename T>
inline T dot(const vec2_t<T>& a, const vec2_t<T>& b) {
    return a.x*b.x + a.y*b.y;
}

inline f32 length(const vec2_t<f32>& a) {
    return sqrtf(a.x*a.x + a.y*a.y);
}

inline f64 length(const vec2_t<f64>& a) {
    return sqrt(a.x*a.x + a.y*a.y);
}

template<typename T>
inline vec2_t<T> normalize(const vec2_t<T>& a) {
    return a / length(a);
}

template<typename T>
inline T square(const T& a) {
    return a*a;
}

template<typename T> inline
T median(T a, T b, T c) {
    if ((b >= a && a >= c)||(c >= a && a >= b))
        return a;
    if ((a >= b && b >= c)||(c >= b && b >= a))
        return b;
    if ((a >= c && c >= b)||(b >= c && c >= a))
        return c;
    return T();
}

template<typename T> inline
T min(T a, T b) {
	return a > b ? b : a;
}

template<typename T> inline
T max(T a, T b) {
	return a > b ? a : b;
}

template<typename T> inline
T clamp(T a, T b, T c) {
    return median(a, b, c);
}

template<typename T>
T angleDiff(T from, T to) {
    T diff = to - from;
    while (diff >= pi)
        diff -= tau;
    while (diff < -pi)
        diff += tau;
    return diff;
}

template<typename T> inline
T angleDir(T from, T to) {
    return angleDiff(from, to) >= 0 ? 1 : -1;
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
    mat2_t<T>() : h{1, 0, 0, 1} {}
    mat2_t<T>(T a) : h{a, 0, 0, a} {}
    mat2_t<T>(T a, T b, T c, T d) : h{a, b, c, d} {}
    inline vec2_t<T> Row1() const { return vec2_t<T>(h.x1, h.y1); }
    inline vec2_t<T> Row2() const { return vec2_t<T>(h.x2, h.y2); }
    inline vec2_t<T> Col1() const { return vec2_t<T>(v.x1, v.y1); }
    inline vec2_t<T> Col2() const { return vec2_t<T>(v.x2, v.y2); }
    mat2_t<float> Rotate(float angle) const {
        float s = sinf(angle), c = cosf(angle);
        return mat2_t<float>(c, s, -s, c) * *this;
    }
    mat2_t<double> Rotate(double angle) const {
        double s = sin(angle), c = cos(angle);
        return mat2_t<double>(c, s, -s, c) * *this;
    }
    mat2_t<T> Skew(vec2_t<T> amount) const {
        return mat2_t<T>(T(1), amount.y, amount.x, T(1)) * *this;
    }
    mat2_t<T> Scale(vec2_t<T> scale) const {
        return mat2_t<T>(scale.x, T(0), T(0), scale.y) * *this;
    }
    inline mat2_t<T> operator+(const mat2_t<T>& a) const {
        return mat2_t<T>(
            h.x1 + a.h.x1,
            h.y1 + a.h.y1,
            h.x2 + a.h.x2,
            h.y2 + a.h.y2
        );
    }
    inline mat2_t<T> operator*(const mat2_t<T>& a) const {
        return mat2_t<T>(
            h.x1 * a.v.x1 + h.y1 * a.v.y1,
            h.x1 * a.v.x2 + h.y1 * a.v.y2,
            h.x2 * a.v.x1 + h.y2 * a.v.y1,
            h.x2 * a.v.x2 + h.y2 * a.v.y2
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
struct vec4_t {
    union {
        struct {
            T x, y, z, w;
        };
        struct {
            T r, g, b, a;
        };
        struct {
            T data[4];
        };
    };

    vec4_t<T>() : x(0) , y(0) , z(0) , w(0) {}
    vec4_t<T>(T v) : x(v) , y (v) , z(v) , w(v) {}
    vec4_t<T>(T v1, T v2, T v3, T v4) : x(v1) , y(v2) , z(v3) , w(v4) {}
    inline vec4_t<T> operator+(const vec4_t<T>& v) const { return vec4_t<T>(x+v.x, y+v.y, z+v.z, w+v.w); }
    inline vec4_t<T> operator-(const vec4_t<T>& v) const { return vec4_t<T>(x-v.x, y-v.y, z-v.z, w-v.w); }
    inline vec4_t<T> operator-() const { return vec4_t<T>(-x, -y, -z, -w); }
    inline vec4_t<T> operator*(const vec4_t<T>& v) const { return vec4_t<T>(x*v.x, y*v.y, z*v.z, w*v.w); }
    inline vec4_t<T> operator/(const vec4_t<T>& v) const { return vec4_t<T>(x/v.x, y/v.y, z/v.z, w/v.w); }
    inline vec4_t<T> operator+=(const vec4_t<T>& v) {
        x += v.x;
        y += v.y;
        z += v.z;
        w += v.w;
        return *this;
    }
    inline vec4_t<T> operator-=(const vec4_t<T>& v) {
        x -= v.x;
        y -= v.y;
        z -= v.z;
        w -= v.w;
        return *this;
    }
    inline vec4_t<T> operator/=(const vec4_t<T>& v) {
        x /= v.x;
        y /= v.y;
        z /= v.z;
        w /= v.w;
        return *this;
    }
    inline vec4_t<T> operator/=(const T& v) {
        x /= v;
        y /= v;
        z /= v;
        w /= v;
        return *this;
    }
    inline vec4_t<T> operator*=(const vec4_t<T>& v) {
        x *= v.x;
        y *= v.y;
        z *= v.z;
        w *= v.w;
        return *this;
    }
    inline vec4_t<T> operator*=(const T& v) {
        x *= v;
        y *= v;
        z *= v;
        w *= v;
        return *this;
    }
    inline vec4_t<T> operator*(const T& v) const { return vec4_t<T>(x*v, y*v, z*v, w*v); }
    inline vec4_t<T> operator/(const T& v) const { return vec4_t<T>(x/v, y/v, z/v, w/v); }
    inline T& operator[](const unsigned i) { return data[i]; }
};

typedef vec2_t<f32> vec2;
typedef vec2_t<f64> vec2d;
typedef vec2_t<i32> vec2i;

typedef vec4_t<f32> vec4;
typedef vec4_t<f64> vec4d;
typedef vec4_t<i32> vec4i;

typedef mat2_t<f32> mat2;
typedef mat2_t<f64> mat2d;

#endif
