/*
    File: mat2_t.hpp
    Author: Philip Haynes
*/

#ifndef AZCORE_MATH_MAT2_T_HPP
#define AZCORE_MATH_MAT2_T_HPP

#include "vec2_t.hpp"

namespace AzCore {

template <typename T>
struct mat2_t
{
    union {
        struct
        {
            T x1, y1,
                x2, y2;
        } h;
        struct
        {
            T x1, x2,
                y1, y2;
        } v;
        struct
        {
            T data[4];
        };
    };
    mat2_t() = default;
    inline mat2_t(const T &a) : h{a, 0, 0, a} {};
    inline mat2_t(const T &a, const T &b, const T &c, const T &d) : h{a, b, c, d} {};
    template <bool rowMajor = true>
    inline mat2_t(const vec2_t<T> &a, const vec2_t<T> &b)
    {
        if constexpr (rowMajor)
        {
            h = {a.x, a.y, b.x, b.y};
        }
        else
        {
            h = {a.x, b.x, a.y, b.y};
        }
    }
    inline mat2_t(const T d[4]) : data{d[0], d[1], d[2], d[3]} {};
    inline vec2_t<T> Row1() const { return vec2_t<T>(h.x1, h.y1); }
    inline vec2_t<T> Row2() const { return vec2_t<T>(h.x2, h.y2); }
    inline vec2_t<T> Col1() const { return vec2_t<T>(v.x1, v.y1); }
    inline vec2_t<T> Col2() const { return vec2_t<T>(v.x2, v.y2); }
    inline static mat2_t<T> Identity()
    {
        return mat2_t(1);
    };
    inline static mat2_t<T> Rotation(T angle)
    {
        T s = sin(angle), c = cos(angle);
        return mat2_t<T>(c, -s, s, c);
    }
    inline static mat2_t<T> Skewer(vec2_t<T> amount)
    {
        return mat2_t<T>(T(1), amount.y, amount.x, T(1));
    }
    inline static mat2_t<T> Scaler(vec2_t<T> scale)
    {
        return mat2_t<T>(scale.x, T(0), T(0), scale.y);
    }
    inline mat2_t<T> Transpose() const
    {
        return mat2_t<T>(v.x1, v.y1, v.x2, v.y2);
    }
    inline mat2_t<T> operator+(const mat2_t<T> &a) const
    {
        return mat2_t<T>(
            h.x1 + a.h.x1, h.y1 + a.h.y1,
            h.x2 + a.h.x2, h.y2 + a.h.y2);
    }
    inline mat2_t<T> operator*(const mat2_t<T> &a) const
    {
        return mat2_t<T>(
            h.x1 * a.v.x1 + h.y1 * a.v.y1, h.x1 * a.v.x2 + h.y1 * a.v.y2,
            h.x2 * a.v.x1 + h.y2 * a.v.y1, h.x2 * a.v.x2 + h.y2 * a.v.y2);
    }
    inline vec2_t<T> operator*(const vec2_t<T> &a) const
    {
        return vec2_t<T>(
            h.x1 * a.x + h.y1 * a.y,
            h.x2 * a.x + h.y2 * a.y);
    }
    inline mat2_t<T> operator*(const T &a) const
    {
        return mat2_t<T>(
            h.x1 * a, h.y1 * a,
            h.x2 * a, h.y2 * a);
    }
};

template <typename T>
inline vec2_t<T> operator*(const vec2_t<T> &a, const mat2_t<T> &b)
{
    return vec2_t<T>(
        a.x * b.v.x1 + a.y * b.v.y1,
        a.x * b.v.x2 + a.y * b.v.y2);
}

} // namespace AzCore

#endif // AZCORE_MATH_MAT2_T_HPP