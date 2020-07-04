/*
    File: mat4_t.hpp
    Author: Philip Haynes
*/

#ifndef AZCORE_MATH_MAT4_T_HPP
#define AZCORE_MATH_MAT4_T_HPP

#include "vec4_t.hpp"

namespace AzCore {

template <typename T>
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
    mat4_t() = default;
    inline mat4_t(const T &a) : h{a, 0, 0, 0, 0, a, 0, 0, 0, 0, a, 0, 0, 0, 0, a} {}
    inline mat4_t(const T &x1, const T &y1, const T &z1, const T &w1,
                  const T &x2, const T &y2, const T &z2, const T &w2,
                  const T &x3, const T &y3, const T &z3, const T &w3,
                  const T &x4, const T &y4, const T &z4, const T &w4) : data{x1, y1, z1, w1, x2, y2, z2, w2, x3, y3, z3, w3, x4, y4, z4, w4} {}
    template <bool rowMajor = true>
    inline mat4_t(const vec4_t<T> &a, const vec4_t<T> &b, const vec4_t<T> &c, const vec4_t<T> &d) {
        if constexpr (rowMajor) {
            h = {a.x, a.y, a.z, a.w, b.x, b.y, b.z, b.w, c.x, c.y, c.z, c.w, d.x, d.y, d.z, d.w};
        } else {
            h = {a.x, b.x, c.x, d.x, a.y, b.y, c.y, d.y, a.z, b.z, c.z, d.z, a.w, b.w, c.w, d.w};
        }
    }
    inline mat4_t(const T d[16]) : data{d[0], d[1], d[2], d[3], d[4], d[5], d[6], d[7], d[9], d[10], d[11], d[12], d[13], d[14], d[15]} {}
    inline vec4_t<T> Row1() const { return vec4_t<T>(h.x1, h.y1, h.z1, h.w1); }
    inline vec4_t<T> Row2() const { return vec4_t<T>(h.x2, h.y2, h.z2, h.w2); }
    inline vec4_t<T> Row3() const { return vec4_t<T>(h.x3, h.y3, h.z3, h.w3); }
    inline vec4_t<T> Row4() const { return vec4_t<T>(h.x4, h.y4, h.z4, h.w4); }
    inline vec4_t<T> Col1() const { return vec4_t<T>(v.x1, v.y1, v.z1, v.w1); }
    inline vec4_t<T> Col2() const { return vec4_t<T>(v.x2, v.y2, v.z2, v.w2); }
    inline vec4_t<T> Col3() const { return vec4_t<T>(v.x3, v.y3, v.z3, v.w3); }
    inline vec4_t<T> Col4() const { return vec4_t<T>(v.x4, v.y4, v.z4, v.w4); }
    inline static mat4_t<T> Identity() {
        return mat4_t(1);
    };
    // Only useful for rotations about aligned planes, such as {{1, 0, 0, 0}, {0, 0, 0, 1}}
    // Note: The planes stay fixed in place and everything else rotates around them.
    static mat4_t<T> RotationBasic(T angle, Plane plane) {
        T s = sin(angle), c = cos(angle);
        switch (plane) {
        case Plane::XW: {
            return mat4_t<T>(
                T(1), T(0), T(0), T(0),
                T(0), c, -s, T(0),
                T(0), s, c, T(0),
                T(0), T(0), T(0), T(1));
        }
        case Plane::YW: {
            return mat4_t<T>(
                c, T(0), s, T(0),
                T(0), T(1), T(0), T(0),
                -s, T(0), c, T(0),
                T(0), T(0), T(0), T(1));
        }
        case Plane::ZW: {
            return mat4_t<T>(
                c, -s, T(0), T(0),
                s, c, T(0), T(0),
                T(0), T(0), T(1), T(0),
                T(0), T(0), T(0), T(1));
        }
        case Plane::XY: {
            return mat4_t<T>(
                T(1), T(0), T(0), T(0),
                T(0), T(1), T(0), T(0),
                T(0), T(0), c, -s,
                T(0), T(0), s, c);
        }
        case Plane::YZ: {
            return mat4_t<T>(
                c, T(0), T(0), -s,
                T(0), T(1), T(0), T(0),
                T(0), T(0), T(1), T(0),
                s, T(0), T(0), c);
        }
        case Plane::ZX: {
            return mat4_t<T>(
                T(1), T(0), T(0), T(0),
                T(0), c, T(0), s,
                T(0), T(0), T(1), T(0),
                T(0), -s, T(0), c);
        }
        }
        return mat4_t<T>();
    }
    // For using 3D-axis rotations
    static mat4_t<T> RotationBasic(T angle, Axis axis) {
        switch (axis) {
        case Axis::X: {
            return RotationBasic(angle, Plane::XW);
        }
        case Axis::Y: {
            return RotationBasic(angle, Plane::YW);
        }
        case Axis::Z: {
            return RotationBasic(angle, Plane::ZW);
        }
        }
        return mat4_t<T>();
    }
    // Useful for arbitrary 3D-axes
    static mat4_t<T> Rotation(T angle, vec3_t<T> axis) {
        T s = sin(angle), c = cos(angle);
        T ic = 1 - c;
        vec3_t<T> a = normalize(axis);
        T xx = square(a.x), yy = square(a.y), zz = square(a.z),
          xy = a.x * a.y, xz = a.x * a.z, yz = a.y * a.z;
        return mat4_t<T>(
            c + xx * ic, xy * ic - a.z * s, xz * ic + a.y * s, T(0),
            xy * ic + a.z * s, c + yy * ic, yz * ic - a.x * s, T(0),
            xz * ic - a.y * s, yz * ic + a.x * s, c + zz * ic, T(0),
            T(0), T(0), T(0), T(1));
    }
    static mat4_t<T> Scaler(vec4_t<T> scale) {
        return mat4_t<T>(scale.x, T(0), T(0), T(0), T(0), scale.y, T(0), T(0), T(0), T(0), scale.z, T(0), T(0), T(0), T(0), scale.w);
    }


    inline mat4_t<T> Transpose() const {
        return mat4_t<T>(v.x1, v.y1, v.z1, v.w1,
                         v.x2, v.y2, v.z2, v.w2,
                         v.x3, v.y3, v.z3, v.w3,
                         v.x4, v.y4, v.z4, v.w4);
    }
    inline mat4_t<T> operator+(const mat4_t<T> &a) const {
        return mat4_t<T>(
            h.x1 + a.h.x1, h.y1 + a.h.y1, h.z1 + a.h.z1, h.w1 + a.h.w1,
            h.x2 + a.h.x2, h.y2 + a.h.y2, h.z2 + a.h.z2, h.w2 + a.h.w2,
            h.x3 + a.h.x3, h.y3 + a.h.y3, h.z3 + a.h.z3, h.w3 + a.h.w3,
            h.x4 + a.h.x4, h.y4 + a.h.y4, h.z4 + a.h.z4, h.w4 + a.h.w4);
    }
    inline mat4_t<T> operator*(const mat4_t<T> &a) const {
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
            h.x4 * a.v.x4 + h.y4 * a.v.y4 + h.z4 * a.v.z4 + h.w4 * a.v.w4);
    }
    inline vec4_t<T> operator*(const vec4_t<T> &a) const {
        return vec4_t<T>(
            h.x1 * a.x + h.y1 * a.y + h.z1 * a.z + h.w1 * a.w,
            h.x2 * a.x + h.y2 * a.y + h.z2 * a.z + h.w2 * a.w,
            h.x3 * a.x + h.y3 * a.y + h.z3 * a.z + h.w3 * a.w,
            h.x4 * a.x + h.y4 * a.y + h.z4 * a.z + h.w4 * a.w);
    }
    inline mat4_t<T> operator*(const T &a) const {
        return mat4_t<T>(
            h.x1 * a, h.y1 * a, h.z1 * a, h.w1 * a,
            h.x2 * a, h.y2 * a, h.z2 * a, h.w2 * a,
            h.x3 * a, h.y3 * a, h.z3 * a, h.w3 * a,
            h.x4 * a, h.y4 * a, h.z4 * a, h.w4 * a);
    }
    inline mat4_t<T> operator/(const T &a) const {
        return mat4_t<T>(
            h.x1 / a, h.y1 / a, h.z1 / a, h.w1 / a,
            h.x2 / a, h.y2 / a, h.z2 / a, h.w2 / a,
            h.x3 / a, h.y3 / a, h.z3 / a, h.w3 / a,
            h.x4 / a, h.y4 / a, h.z4 / a, h.w4 / a);
    }
};

template <typename T>
inline vec4_t<T> operator*(const vec4_t<T> &a, const mat4_t<T> &b) {
    return vec4_t<T>(
        a.x * b.v.x1 + a.y * b.v.y1 + a.z * b.v.z1 + a.w * b.v.w1,
        a.x * b.v.x2 + a.y * b.v.y2 + a.z * b.v.z2 + a.w * b.v.w2,
        a.x * b.v.x3 + a.y * b.v.y3 + a.z * b.v.z3 + a.w * b.v.w3,
        a.x * b.v.x4 + a.y * b.v.y4 + a.z * b.v.z4 + a.w * b.v.w4);
}

} // namespace AzCore

#endif // AZCORE_MATH_MAT4_T_HPP
