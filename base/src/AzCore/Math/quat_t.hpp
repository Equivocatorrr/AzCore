/*
    File: quat_t.hpp
    Author: Philip Haynes
*/

#ifndef AZCORE_MATH_QUAT_T_HPP
#define AZCORE_MATH_QUAT_T_HPP

#include "mat3_t.hpp"
#include "vec3_t.hpp"
#include "vec4_t.hpp"

#include "basic.hpp"

namespace AzCore {

template <typename T>
struct quat_t
{
    union {
        struct
        {
            T w, x, y, z;
        };
        struct
        {
            T scalar;
            vec3_t<T> vector;
        };
        struct
        {
            vec4_t<T> wxyz;
        };
        struct
        {
            T data[4];
        };
    };

    quat_t() = default;
    inline quat_t(const T &a) : data{a, 0, 0, 0} {}
    inline quat_t(const T &a, const vec3_t<T> &v) : scalar(a), vector(v) {}
    inline quat_t(const vec4_t<T> &v) : wxyz(v)
    {
    }
    inline quat_t(const T &a, const T &b, const T &c, const T &d) : w(a), x(b), y(c), z(d)
    {
    }
    inline quat_t(const T d[4]) : data{d[0], d[1], d[2], d[3]} {}

    inline quat_t<T> operator*(const quat_t<T> &a) const
    {
        return quat_t<T>(
            w * a.w - x * a.x - y * a.y - z * a.z,
            w * a.x + x * a.w + y * a.z - z * a.y,
            w * a.y - x * a.z + y * a.w + z * a.x,
            w * a.z + x * a.y - y * a.x + z * a.w);
    }
    inline quat_t<T> operator*(const T &a) const
    {
        return quat_t<T>(w * a, x * a, y * a, z * a);
    }
    inline quat_t<T> operator/(const quat_t<T> &a) const
    {
        return (*this) * a.Reciprocal();
    }
    inline quat_t<T> operator/(const T &a) const
    {
        return quat_t<T>(w / a, x / a, y / a, z / a);
    }
    inline quat_t<T> operator-(const quat_t<T> &a) const
    {
        return quat_t<T>(w - a.w, x - a.x, y - a.y, z - a.z);
    }
    inline quat_t<T> operator+(const quat_t<T> &a) const
    {
        return quat_t<T>(w + a.w, x + a.x, y + a.y, z + a.z);
    }
    inline quat_t<T>& operator+=(const quat_t<T> &a)
    {
        w += a.w;
        x += a.x;
        y += a.y;
        z += a.z;
        return *this;
    }
    inline quat_t<T>& operator-=(const quat_t<T> &a)
    {
        w -= a.w;
        x -= a.x;
        y -= a.y;
        z -= a.z;
        return *this;
    }
    inline quat_t<T>& operator*=(const quat_t<T> &a)
    {
        *this = *this * a;
        return *this;
    }
    inline quat_t<T>& operator/=(const quat_t<T> &a)
    {
        *this = *this / a;
        return *this;
    }
    inline quat_t<T>& operator*=(const T &a)
    {
        *this = *this * a;
        return *this;
    }
    inline quat_t<T>& operator/=(const T &a)
    {
        *this = *this / a;
        return *this;
    }
    inline quat_t<T> Conjugate() const
    {
        return quat_t<T>(scalar, -vector);
    }
    inline T Norm() const
    {
        return sqrt(w * w + x * x + y * y + z * z);
    }
    inline quat_t<T> Reciprocal() const
    {
        return Conjugate() / (w * w + x * x + y * y + z * z); // For unit quaternions just use Conjugate()
    }
    // Make a rotation quaternion
    static inline quat_t<T> Rotation(const T &angle, const vec3_t<T> &axis)
    {
        return quat_t<T>(cos(angle / T(2.0)), normalize(axis) * sin(angle / T(2.0)));
    }
    // A one-off rotation of a point
    static vec3_t<T> RotatePoint(vec3_t<T> point, T angle, vec3_t<T> axis)
    {
        quat_t<T> rot = Rotation(angle, axis);
        rot = rot * quat_t<T>(T(0), point) * rot.Conjugate();
        return rot.vector;
    }
    // Using this quaternion for a one-off rotation of a point
    vec3_t<T> RotatePoint(vec3_t<T> point) const
    {
        return ((*this) * quat_t<T>(T(0), point) * Conjugate()).vector;
    }
    // Rotating this quaternion about an axis
    quat_t<T> Rotate(T angle, vec3_t<T> axis) const
    {
        quat_t<T> rot = Rotation(angle, axis);
        return rot * (*this) * rot.Conjugate();
    }
    // Rotate this quaternion by using a specified rotation quaternion
    quat_t<T> Rotate(quat_t<T> rotation) const
    {
        return rotation * (*this) * rotation.Conjugate();
    }
    // Convert this rotation quaternion into a matrix
    mat3_t<T> ToMat3() const
    {
        const T ir = w * x, jr = w * y, kr = w * z,
                ii = x * x, ij = x * y, ik = x * z,
                jj = y * y, jk = y * z,
                kk = z * z;
        return mat3_t<T>(
            1 - 2 * (jj + kk), 2 * (ij - kr), 2 * (ik + jr),
            2 * (ij + kr), 1 - 2 * (ii + kk), 2 * (jk - ir),
            2 * (ik - jr), 2 * (jk + ir), 1 - 2 * (ii + jj));
    }
};

} // namespace AzCore

template <typename T>
inline AzCore::quat_t<T> normalize(const AzCore::quat_t<T> &a)
{
    return a / a.Norm();
}

template <typename T>
AzCore::quat_t<T> slerp(AzCore::quat_t<T> a, AzCore::quat_t<T> b, T factor)
{
    a = normalize(a);
    b = normalize(b);
    T d = dot(a.vector, b.vector);
    if (d < T(0.0))
    {
        b = -b.wxyz;
        d *= T(-1.0);
    }
    const T threshold = T(0.999);
    if (d > threshold)
    {
        return normalize(a + (b - a) * factor);
    }
    T thetaMax = acos(d);
    T theta = thetaMax * factor;
    T base = sin(theta) / sin(thetaMax);
    return a * (cos(theta) - d * base) + b * base;
}

template <typename T>
AzCore::quat_t<T> exp(AzCore::quat_t<T> a)
{
    T theta = abs(a.vector);
    return AzCore::quat_t<T>(cos(theta), theta > T(0.0000001) ? (a.vector * sin(theta) / theta) : AzCore::vec3_t<T>(0)) * exp(a.scalar);
}

template <typename T>
AzCore::quat_t<T> log(AzCore::quat_t<T> a)
{
    // if (a.scalar < 0)
    //     a *= -1;
    T len = log(a.Norm());
    T vLen = abs(a.vector);
    T theta = atan2(vLen, a.scalar);
    return AzCore::quat_t<T>(len, vLen > T(0.0000001) ? (a.vector / vLen * theta) : AzCore::vec3_t<T>(theta, 0, 0));
}

template <typename T>
AzCore::quat_t<T> pow(const AzCore::quat_t<T> &a, const AzCore::quat_t<T> &e)
{
    return exp(log(a) * e);
}

template <typename T>
AzCore::quat_t<T> pow(const AzCore::quat_t<T> &a, const T &e)
{
    return exp(log(a) * e);
}

#endif // AZCORE_MATH_QUAT_T_HPP
