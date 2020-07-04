/*
    File: mat3_t.hpp
    Author: Philip Haynes
*/

#ifndef AZCORE_MATH_MAT3_T_HPP
#define AZCORE_MATH_MAT3_T_HPP

#include "vec3_t.hpp"

namespace AzCore {

template <typename T>
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
    mat3_t() = default;
    inline mat3_t(const T &a) : h{a, 0, 0, 0, a, 0, 0, 0, a} {}
    inline mat3_t(const T &x1, const T &y1, const T &z1,
                  const T &x2, const T &y2, const T &z2,
                  const T &x3, const T &y3, const T &z3) : data{x1, y1, z1, x2, y2, z2, x3, y3, z3} {}
    template <bool rowMajor = true>
    inline mat3_t(const vec3_t<T> &a, const vec3_t<T> &b, const vec3_t<T> &c) {
        if constexpr (rowMajor) {
            h = {a.x, a.y, a.z, b.x, b.y, b.z, c.x, c.y, c.z};
        } else {
            h = {a.x, b.x, c.x, a.y, b.y, c.y, a.z, b.z, c.z};
        }
    }
    inline mat3_t(const T d[9]) : data{d[0], d[1], d[2], d[3], d[4], d[5], d[6], d[7], d[9]} {}
    inline vec3_t<T> Row1() const { return vec3_t<T>(h.x1, h.y1, h.z1); }
    inline vec3_t<T> Row2() const { return vec3_t<T>(h.x2, h.y2, h.z2); }
    inline vec3_t<T> Row3() const { return vec3_t<T>(h.x3, h.y3, h.z3); }
    inline vec3_t<T> Col1() const { return vec3_t<T>(v.x1, v.y1, v.z1); }
    inline vec3_t<T> Col2() const { return vec3_t<T>(v.x2, v.y2, v.z2); }
    inline vec3_t<T> Col3() const { return vec3_t<T>(v.x3, v.y3, v.z3); }
    inline static mat3_t<T> Identity() {
        return mat3_t(1);
    };
    // Only useful for rotations about aligned axes, such as {1, 0, 0}
    static mat3_t<T> RotationBasic(T angle, Axis axis) {
        T s = sin(angle), c = cos(angle);
        switch (axis) {
        case Axis::X: {
            return mat3_t<T>(
                T(1), T(0), T(0),
                T(0), c, -s,
                T(0), s, c);
        }
        case Axis::Y: {
            return mat3_t<T>(
                c, T(0), s,
                T(0), T(1), T(0),
                -s, T(0), c);
        }
        case Axis::Z: {
            return mat3_t<T>(
                c, -s, T(0),
                s, c, T(0),
                T(0), T(0), T(1));
        }
        }
        return mat3_t<T>();
    }
    // Useful for arbitrary axes
    static mat3_t<T> Rotation(T angle, vec3_t<T> axis) {
        T s = sin(angle), c = cos(angle);
        T ic = 1 - c;
        vec3_t<T> a = normalize(axis);
        T xx = square(a.x), yy = square(a.y), zz = square(a.z),
          xy = a.x * a.y, xz = a.x * a.z, yz = a.y * a.z;
        return mat3_t<T>(
            c + xx * ic, xy * ic - a.z * s, xz * ic + a.y * s,
            xy * ic + a.z * s, c + yy * ic, yz * ic - a.x * s,
            xz * ic - a.y * s, yz * ic + a.x * s, c + zz * ic);
    }
    static mat3_t<T> Scaler(vec3_t<T> scale) {
        return mat3_t<T>(scale.x, T(0), T(0), T(0), scale.y, T(0), T(0), T(0), scale.z);
    }
    inline mat3_t<T> Transpose() const {
        return mat3_t<T>(v.x1, v.y1, v.z1, v.x2, v.y2, v.z2, v.x3, v.y3, v.z3);
    }
    inline mat3_t<T> operator+(const mat3_t<T> &a) const {
        return mat3_t<T>(
            h.x1 + a.h.x1, h.y1 + a.h.y1, h.z1 + a.h.z1,
            h.x2 + a.h.x2, h.y2 + a.h.y2, h.z2 + a.h.z2,
            h.x3 + a.h.x3, h.y3 + a.h.y3, h.z3 + a.h.z3);
    }
    inline mat3_t<T> operator*(const mat3_t<T> &a) const {
        return mat3_t<T>(
            h.x1 * a.v.x1 + h.y1 * a.v.y1 + h.z1 * a.v.z1,
            h.x1 * a.v.x2 + h.y1 * a.v.y2 + h.z1 * a.v.z2,
            h.x1 * a.v.x3 + h.y1 * a.v.y3 + h.z1 * a.v.z3,
            h.x2 * a.v.x1 + h.y2 * a.v.y1 + h.z2 * a.v.z1,
            h.x2 * a.v.x2 + h.y2 * a.v.y2 + h.z2 * a.v.z2,
            h.x2 * a.v.x3 + h.y2 * a.v.y3 + h.z2 * a.v.z3,
            h.x3 * a.v.x1 + h.y3 * a.v.y1 + h.z3 * a.v.z1,
            h.x3 * a.v.x2 + h.y3 * a.v.y2 + h.z3 * a.v.z2,
            h.x3 * a.v.x3 + h.y3 * a.v.y3 + h.z3 * a.v.z3);
    }
    inline vec3_t<T> operator*(const vec3_t<T> &a) const {
        return vec3_t<T>(
            h.x1 * a.x + h.y1 * a.y + h.z1 * a.z,
            h.x2 * a.x + h.y2 * a.y + h.z2 * a.z,
            h.x3 * a.x + h.y3 * a.y + h.z3 * a.z);
    }
    inline mat3_t<T> operator*(const T &a) const {
        return mat3_t<T>(
            h.x1 * a, h.y1 * a, h.z1 * a,
            h.x2 * a, h.y2 * a, h.z2 * a,
            h.x3 * a, h.y3 * a, h.z3 * a);
    }
    inline mat3_t<T> operator/(const T &a) const {
        return mat3_t<T>(
            h.x1 / a, h.y1 / a, h.z1 / a,
            h.x2 / a, h.y2 / a, h.z2 / a,
            h.x3 / a, h.y3 / a, h.z3 / a);
    }
};

template <typename T>
inline vec3_t<T> operator*(const vec3_t<T> &a, const mat3_t<T> &b) {
    return vec3_t<T>(
        a.x * b.v.x1 + a.y * b.v.y1 + a.z * b.v.z1,
        a.x * b.v.x2 + a.y * b.v.y2 + a.z * b.v.z2,
        a.x * b.v.x3 + a.y * b.v.y3 + a.z * b.v.z3);
}

} // namespace AzCore

#endif // AZCORE_MATH_MAT3_T_HPP
