/*
    File: math.hpp
    Author: Philip Haynes
    Description: Common math routines and data types.
    Note: Vector math is right-handed.
*/
#ifndef MATH_HPP
#define MATH_HPP

#include "basictypes.hpp"

#include <cmath>

const f32 halfpi = 1.5707963267948966;
const f32 pi = 3.1415926535897932;
const f32 tau = 6.2831853071795865;

enum Axis {
    X=0, Y=1, Z=2, W=3
};

inline f32 cos(const f32& a) { return cosf(a); }
inline f32 sin(const f32& a) { return sinf(a); }
inline f32 tan(const f32& a) { return tanf(a); }
inline f32 asin(const f32& a) { return asinf(a); }
inline f32 acos(const f32& a) { return acosf(a); }
inline f32 atan(const f32& a) { return atanf(a); }
inline f32 atan2(const f32& y, const f32& x) { return atan2f(y, x); }
inline f32 sqrt(const f32& a) { return sqrtf(a); }

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
T angleDiff(T from, T to) {
    T diff = to - from;
    while (diff >= pi)
        diff -= tau;
    while (diff < -pi)
        diff += tau;
    return diff;
}

template<typename T>
inline T angleDir(const T& from, const T& to) {
    return angleDiff(from, to) >= 0 ? 1 : -1;
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

    vec2_t<T>() : x(0) , y(0) {}
    vec2_t<T>(T a) : x(a) , y (a) {}
    vec2_t<T>(T a, T b) : x(a) , y(b) {}
    inline vec2_t<T> operator+(const vec2_t<T>& a) const { return vec2_t<T>(x+a.x, y+a.y); }
    inline vec2_t<T> operator-(const vec2_t<T>& a) const { return vec2_t<T>(x-a.x, y-a.y); }
    inline vec2_t<T> operator-() const { return vec2_t<T>(-x, -y); }
    inline vec2_t<T> operator*(const vec2_t<T>& a) const { return vec2_t<T>(x*a.x, y*a.y); }
    inline vec2_t<T> operator/(const vec2_t<T>& a) const { return vec2_t<T>(x/a.x, y/a.y); }
    inline vec2_t<T> operator*(const T& a) const { return vec2_t<T>(x*a, y*a); }
    inline vec2_t<T> operator/(const T& a) const { return vec2_t<T>(x/a, y/a); }
    inline T& operator[](const unsigned& i) { return data[i]; }
    vec2_t<T> operator+=(const vec2_t<T>& a) {
        x += a.x;
        y += a.y;
        return *this;
    }
    vec2_t<T> operator-=(const vec2_t<T>& a) {
        x -= a.x;
        y -= a.y;
        return *this;
    }
    vec2_t<T> operator/=(const vec2_t<T>& a) {
        x /= a.x;
        y /= a.y;
        return *this;
    }
    vec2_t<T> operator/=(const T& a) {
        x /= a;
        y /= a;
        return *this;
    }
    vec2_t<T> operator*=(const vec2_t<T>& a) {
        x *= a.x;
        y *= a.y;
        return *this;
    }
    vec2_t<T> operator*=(const T& a) {
        x *= a;
        y *= a;
        return *this;
    }
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
    mat2_t<T>() : h{1, 0, 0, 1} {}
    mat2_t<T>(T a) : h{a, 0, 0, a} {}
    mat2_t<T>(T a, T b, T c, T d) : h{a, b, c, d} {}
    mat2_t<T>(vec2_t<T> a, vec2_t<T> b, bool rowMajor=true) {
        if (rowMajor) {
            h = {a.x, a.y, b.x, b.y};
        } else {
            h = {a.x, b.x, a.y, b.y};
        }
    }
    mat2_t<T>(T d[4]) : data{d} {}
    inline vec2_t<T> Row1() const { return vec2_t<T>(h.x1, h.y1); }
    inline vec2_t<T> Row2() const { return vec2_t<T>(h.x2, h.y2); }
    inline vec2_t<T> Col1() const { return vec2_t<T>(v.x1, v.y1); }
    inline vec2_t<T> Col2() const { return vec2_t<T>(v.x2, v.y2); }
    static mat2_t<T> Rotation(T angle) {
        T s = sin(angle), c = cos(angle);
        return mat2_t<T>(c, -s, s, c);
    }
    inline mat2_t<T> Rotate(const T& angle) const {
        return mat2_t<T>::Rotation(angle) * (*this);
    }
    static mat2_t<T> Skewer(vec2_t<T> amount) {
        return mat2_t<T>(T(1), amount.y, amount.x, T(1));
    }
    inline mat2_t<T> Skew(const vec2_t<T>& amount) const {
        return mat2_t<T>::Skewer(amount) * (*this);
    }
    static mat2_t<T> Scaler(vec2_t<T> scale) {
        return mat2_t<T>(scale.x, T(0), T(0), scale.y);
    }
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

    vec3_t<T>() : x(0) , y(0) , z(0) {}
    vec3_t<T>(T v) : x(v) , y (v) , z(v) {}
    vec3_t<T>(T v1, T v2, T v3) : x(v1) , y(v2) , z(v3) {}
    inline vec3_t<T> operator+(const vec3_t<T>& v) const { return vec3_t<T>(x+v.x, y+v.y, z+v.z); }
    inline vec3_t<T> operator-(const vec3_t<T>& v) const { return vec3_t<T>(x-v.x, y-v.y, z-v.z); }
    inline vec3_t<T> operator-() const { return vec3_t<T>(-x, -y, -z); }
    inline vec3_t<T> operator*(const vec3_t<T>& v) const { return vec3_t<T>(x*v.x, y*v.y, z*v.z); }
    inline vec3_t<T> operator/(const vec3_t<T>& v) const { return vec3_t<T>(x/v.x, y/v.y, z/v.z); }
    inline vec3_t<T> operator*(const T& v) const { return vec3_t<T>(x*v, y*v, z*v); }
    inline vec3_t<T> operator/(const T& v) const { return vec3_t<T>(x/v, y/v, z/v); }
    inline T& operator[](const unsigned& i) { return data[i]; }
    vec3_t<T> operator+=(const vec3_t<T>& v) {
        x += v.x;
        y += v.y;
        z += v.z;
        return *this;
    }
    vec3_t<T> operator-=(const vec3_t<T>& v) {
        x -= v.x;
        y -= v.y;
        z -= v.z;
        return *this;
    }
    vec3_t<T> operator/=(const vec3_t<T>& v) {
        x /= v.x;
        y /= v.y;
        z /= v.z;
        return *this;
    }
    vec3_t<T> operator/=(const T& v) {
        x /= v;
        y /= v;
        z /= v;
        return *this;
    }
    vec3_t<T> operator*=(const vec3_t<T>& v) {
        x *= v.x;
        y *= v.y;
        z *= v.z;
        return *this;
    }
    vec3_t<T> operator*=(const T& v) {
        x *= v;
        y *= v;
        z *= v;
        return *this;
    }
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
    mat3_t<T>() : h{1, 0, 0, 0, 1, 0, 0, 0, 1} {}
    mat3_t<T>(T a) : h{a, 0, 0, 0, a, 0, 0, 0, a} {}
    mat3_t<T>(T x1, T y1, T z1, T x2, T y2, T z2, T x3, T y3, T z3) : data{x1, y1, z1, x2, y2, z2, x3, y3, z3} {}
    mat3_t<T>(vec3_t<T> a, vec3_t<T> b, vec3_t<T> c, bool rowMajor=true) {
        if (rowMajor) {
            h = {a.x, a.y, a.z, b.x, b.y, b.z, c.x, c.y, c.z};
        } else {
            h = {a.x, b.x, c.x, a.y, b.y, c.y, a.z, b.z, c.z};
        }
    }
    mat3_t<T>(T d[9]) : data{d} {}
    inline vec3_t<T> Row1() const { return vec3_t<T>(h.x1, h.y1, h.z1); }
    inline vec3_t<T> Row2() const { return vec3_t<T>(h.x2, h.y2, h.z2); }
    inline vec3_t<T> Row3() const { return vec3_t<T>(h.x3, h.y3, h.z3); }
    inline vec3_t<T> Col1() const { return vec3_t<T>(v.x1, v.y1, v.z1); }
    inline vec3_t<T> Col2() const { return vec3_t<T>(v.x2, v.y2, v.z2); }
    inline vec3_t<T> Col3() const { return vec3_t<T>(v.x3, v.y3, v.z3); }
    // Only useful for rotations about aligned axes, such as {1, 0, 0}
    static mat3_t<T> RotationBasic(T angle, Axis axis) {
        T s = sin(angle), c = cos(angle);
        switch(axis) {
            case Axis::X: {
                return mat3_t<T>(
                    T(1), T(0), T(0),
                    T(0), c,    -s,
                    T(0), s,    c
                );
            }
            case Axis::Y: {
                return mat3_t<T>(
                    c,    T(0), s,
                    T(0), T(1), T(0),
                    -s,   T(0), c
                );
            }
            case Axis::Z: {
                return mat3_t<T>(
                    c,    -s,   T(0),
                    s,    c,    T(0),
                    T(0), T(0), T(1)
                );
            }
            default:
                return mat3_t<T>();
        }
    }
    inline mat3_t<T> RotateBasic(const T& angle, const Axis& axis) const {
        return mat3_t<T>::RotationBasic(angle, axis) * (*this);
    }
    // Useful for arbitrary axes
    static mat3_t<T> Rotation(T angle, vec3_t<T> axis) {
        T s = sin(angle), c = cos(angle);
        T ic = 1-c;
        vec3_t<T> a = normalize(axis);
        T xx = square(a.x), yy = square(a.y), zz = square(a.z),
            xy = a.x*a.y,     xz = a.x*a.z,     yz = a.y*a.z;
        return mat3_t<T>(
            c + xx*ic,          xy*ic - a.z*s,      xz*ic + a.y*s,
            xy*ic + a.z*s,      c + yy*ic,          yz*ic - a.x*s,
            xz*ic - a.y*s,      yz*ic + a.x*s,      c + zz*ic
        );
    }
    inline mat3_t<T> Rotate(const T& angle, const vec3_t<T> axis) const {
        return mat3_t<T>::Rotation(angle, axis) * (*this);
    }
    static mat3_t<T> Scaler(vec3_t<T> scale) {
        return mat3_t<T>(scale.x, T(0), T(0), T(0), scale.y, T(0), T(0), T(0), scale.z);
    }
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
        return vec3_t<T>(
            h.x1/a.x + h.y1/a.y + h.z1/a.z,
            h.x2/a.x + h.y2/a.y + h.z2/a.z,
            h.x3/a.x + h.y3/a.y + h.z3/a.z
        );
    }
    inline mat3_t<T> operator/(const T& a) const {
        return vec3_t<T>(
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
    inline vec4_t<T> operator*(const T& v) const { return vec4_t<T>(x*v, y*v, z*v, w*v); }
    inline vec4_t<T> operator/(const T& v) const { return vec4_t<T>(x/v, y/v, z/v, w/v); }
    inline T& operator[](const unsigned& i) { return data[i]; }
    vec4_t<T> operator+=(const vec4_t<T>& v) {
        x += v.x;
        y += v.y;
        z += v.z;
        w += v.w;
        return *this;
    }
    vec4_t<T> operator-=(const vec4_t<T>& v) {
        x -= v.x;
        y -= v.y;
        z -= v.z;
        w -= v.w;
        return *this;
    }
    vec4_t<T> operator/=(const vec4_t<T>& v) {
        x /= v.x;
        y /= v.y;
        z /= v.z;
        w /= v.w;
        return *this;
    }
    vec4_t<T> operator/=(const T& v) {
        x /= v;
        y /= v;
        z /= v;
        w /= v;
        return *this;
    }
    vec4_t<T> operator*=(const vec4_t<T>& v) {
        x *= v.x;
        y *= v.y;
        z *= v.z;
        w *= v.w;
        return *this;
    }
    vec4_t<T> operator*=(const T& v) {
        x *= v;
        y *= v;
        z *= v;
        w *= v;
        return *this;
    }
};

template<typename T>
inline T dot(const vec4_t<T>& a, const vec4_t<T>& b) {
    return a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w;
}

template<typename T>
inline T length(const vec4_t<T>& a) {
    return sqrt(a.x*a.x + a.y*a.y + a.z*a.z + a.w*a.w);
}

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

#endif
