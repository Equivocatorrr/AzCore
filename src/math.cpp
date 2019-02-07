/*
    File: math.cpp
    Author: Philip Haynes
*/

#include "math.hpp"

RandomNumberGenerator::RandomNumberGenerator() {
    Seed(Clock::now().time_since_epoch().count());
}

u32 RandomNumberGenerator::Generate() {
    u64 t;
    x = 314527869 * x + 1234567;
    y ^= y << 5;
    y ^= y >> 7;
    y ^= y << 22;
    t = 4294584393ULL * (u64)z + (u64)c;
    c = t >> 32; z = t;
    return x + y + z;
}

void RandomNumberGenerator::Seed(u64 seed) {
    // The power of keysmashes!
    if (seed == 0)
        seed += 3478596;
    x = seed;
    y = seed * 16807;
    z = seed * 47628;
    c = seed * 32497;
}

#ifdef MATH_F32
    f32 angleDiff(f32 from, f32 to) {
        f32 diff = to - from;
        while (diff >= pi)
            diff -= tau;
        while (diff < -pi)
            diff += tau;
        return diff;
    }
#endif
#ifdef MATH_F64
    f64 angleDiff(f64 from, f64 to) {
        f64 diff = to - from;
        while (diff >= pi64)
            diff -= tau64;
        while (diff < -pi64)
            diff += tau64;
        return diff;
    }
#endif

// Template stuff is pretty ugly, but it saves a lot of lines.
#ifdef MATH_VEC2
    template<typename T>
    vec2_t<T>::vec2_t() : x(0) , y(0) {}

    template<typename T>
    vec2_t<T>::vec2_t(T a) : x(a) , y (a) {}

    template<typename T>
    vec2_t<T>::vec2_t(T a, T b) : x(a) , y(b) {}

    template<typename T>
    vec2_t<T> vec2_t<T>::operator+=(const vec2_t<T>& a) {
        x += a.x;
        y += a.y;
        return *this;
    }

    template<typename T>
    vec2_t<T> vec2_t<T>::operator-=(const vec2_t<T>& a) {
        x -= a.x;
        y -= a.y;
        return *this;
    }

    template<typename T>
    vec2_t<T> vec2_t<T>::operator/=(const vec2_t<T>& a) {
        x /= a.x;
        y /= a.y;
        return *this;
    }

    template<typename T>
    vec2_t<T> vec2_t<T>::operator/=(const T& a) {
        x /= a;
        y /= a;
        return *this;
    }

    template<typename T>
    vec2_t<T> vec2_t<T>::operator*=(const vec2_t<T>& a) {
        x *= a.x;
        y *= a.y;
        return *this;
    }

    template<typename T>
    vec2_t<T> vec2_t<T>::operator*=(const T& a) {
        x *= a;
        y *= a;
        return *this;
    }

#endif // MATH_VEC2

#ifdef MATH_MAT2
    template<typename T>
    mat2_t<T>::mat2_t() : h{1, 0, 0, 1} {}

    template<typename T>
    mat2_t<T>::mat2_t(T a) : h{a, 0, 0, a} {}

    template<typename T>
    mat2_t<T>::mat2_t(T a, T b, T c, T d) : h{a, b, c, d} {}

    template<typename T>
    mat2_t<T>::mat2_t(vec2_t<T> a, vec2_t<T> b, bool rowMajor) {
        if (rowMajor) {
            h = {a.x, a.y, b.x, b.y};
        } else {
            h = {a.x, b.x, a.y, b.y};
        }
    }

    template<typename T>
    mat2_t<T>::mat2_t(T d[4]) : data{d[0], d[1], d[2], d[3]} {}

    template<typename T>
    mat2_t<T> mat2_t<T>::Rotation(T angle) {
        T s = sin(angle), c = cos(angle);
        return mat2_t<T>(c, -s, s, c);
    }

    template<typename T>
    mat2_t<T> mat2_t<T>::Skewer(vec2_t<T> amount) {
        return mat2_t<T>(T(1), amount.y, amount.x, T(1));
    }

    template<typename T>
    mat2_t<T> mat2_t<T>::Scaler(vec2_t<T> scale) {
        return mat2_t<T>(scale.x, T(0), T(0), scale.y);
    }

#endif // MATH_MAT2

#ifdef MATH_VEC3
    template<typename T>
    vec3_t<T>::vec3_t() : x(0) , y(0) , z(0) {}

    template<typename T>
    vec3_t<T>::vec3_t(T v) : x(v) , y (v) , z(v) {}

    template<typename T>
    vec3_t<T>::vec3_t(T v1, T v2, T v3) : x(v1) , y(v2) , z(v3) {}

    template<typename T>
    vec3_t<T> vec3_t<T>::operator+=(const vec3_t<T>& v) {
        x += v.x;
        y += v.y;
        z += v.z;
        return *this;
    }

    template<typename T>
    vec3_t<T> vec3_t<T>::operator-=(const vec3_t<T>& v) {
        x -= v.x;
        y -= v.y;
        z -= v.z;
        return *this;
    }

    template<typename T>
    vec3_t<T> vec3_t<T>::operator/=(const vec3_t<T>& v) {
        x /= v.x;
        y /= v.y;
        z /= v.z;
        return *this;
    }

    template<typename T>
    vec3_t<T> vec3_t<T>::operator/=(const T& v) {
        x /= v;
        y /= v;
        z /= v;
        return *this;
    }

    template<typename T>
    vec3_t<T> vec3_t<T>::operator*=(const vec3_t<T>& v) {
        x *= v.x;
        y *= v.y;
        z *= v.z;
        return *this;
    }

    template<typename T>
    vec3_t<T> vec3_t<T>::operator*=(const T& v) {
        x *= v;
        y *= v;
        z *= v;
        return *this;
    }

#endif // MATH_VEC3

#ifdef MATH_MAT3
    template<typename T>
    mat3_t<T>::mat3_t() : h{1, 0, 0, 0, 1, 0, 0, 0, 1} {}

    template<typename T>
    mat3_t<T>::mat3_t(T a) : h{a, 0, 0, 0, a, 0, 0, 0, a} {}

    template<typename T>
    mat3_t<T>::mat3_t(T x1, T y1, T z1, T x2, T y2, T z2, T x3, T y3, T z3) : data{x1, y1, z1, x2, y2, z2, x3, y3, z3} {}

    template<typename T>
    mat3_t<T>::mat3_t(vec3_t<T> a, vec3_t<T> b, vec3_t<T> c, bool rowMajor) {
        if (rowMajor) {
            h = {a.x, a.y, a.z, b.x, b.y, b.z, c.x, c.y, c.z};
        } else {
            h = {a.x, b.x, c.x, a.y, b.y, c.y, a.z, b.z, c.z};
        }
    }

    template<typename T>
    mat3_t<T>::mat3_t(T d[9]) : data{d[0], d[1], d[2], d[3], d[4], d[5], d[6], d[7], d[9]} {}

    template<typename T>
    mat3_t<T> mat3_t<T>::RotationBasic(T angle, Axis axis) {
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
        }
        return mat3_t<T>();
    }

    template<typename T>
    mat3_t<T> mat3_t<T>::Rotation(T angle, vec3_t<T> axis) {
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

    template<typename T>
    mat3_t<T> mat3_t<T>::Scaler(vec3_t<T> scale) {
        return mat3_t<T>(scale.x, T(0), T(0), T(0), scale.y, T(0), T(0), T(0), scale.z);
    }

#endif // MATH_MAT3

#ifdef MATH_VEC4
    template<typename T>
    vec4_t<T>::vec4_t() : x(0) , y(0) , z(0) , w(0) {}

    template<typename T>
    vec4_t<T>::vec4_t(T v) : x(v) , y (v) , z(v) , w(v) {}

    template<typename T>
    vec4_t<T>::vec4_t(T v1, T v2, T v3, T v4) : x(v1) , y(v2) , z(v3) , w(v4) {}

    template<typename T>
    vec4_t<T> vec4_t<T>::operator+=(const vec4_t<T>& v) {
        x += v.x;
        y += v.y;
        z += v.z;
        w += v.w;
        return *this;
    }

    template<typename T>
    vec4_t<T> vec4_t<T>::operator-=(const vec4_t<T>& v) {
        x -= v.x;
        y -= v.y;
        z -= v.z;
        w -= v.w;
        return *this;
    }

    template<typename T>
    vec4_t<T> vec4_t<T>::operator/=(const vec4_t<T>& v) {
        x /= v.x;
        y /= v.y;
        z /= v.z;
        w /= v.w;
        return *this;
    }

    template<typename T>
    vec4_t<T> vec4_t<T>::operator/=(const T& v) {
        x /= v;
        y /= v;
        z /= v;
        w /= v;
        return *this;
    }

    template<typename T>
    vec4_t<T> vec4_t<T>::operator*=(const vec4_t<T>& v) {
        x *= v.x;
        y *= v.y;
        z *= v.z;
        w *= v.w;
        return *this;
    }

    template<typename T>
    vec4_t<T> vec4_t<T>::operator*=(const T& v) {
        x *= v;
        y *= v;
        z *= v;
        w *= v;
        return *this;
    }

#endif // MATH_VEC4

#ifdef MATH_MAT4
    template<typename T>
    mat4_t<T>::mat4_t() : h{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1} {}

    template<typename T>
    mat4_t<T>::mat4_t(T a) : h{a, 0, 0, 0, 0, a, 0, 0, 0, 0, a, 0, 0, 0, 0, a} {}

    template<typename T>
    mat4_t<T>::mat4_t(T x1, T y1, T z1, T w1,
                      T x2, T y2, T z2, T w2,
                      T x3, T y3, T z3, T w3,
                      T x4, T y4, T z4, T w4) :
            data{x1, y1, z1, w1, x2, y2, z2, w2, x3, y3, z3, w3, x4, y4, z4, w4} {}

    template<typename T>
    mat4_t<T>::mat4_t(vec4_t<T> a, vec4_t<T> b, vec4_t<T> c, vec4_t<T> d, bool rowMajor) {
        if (rowMajor) {
            h = {a.x, a.y, a.z, a.w, b.x, b.y, b.z, b.w, c.x, c.y, c.z, c.w, d.x, d.y, d.z, d.w};
        } else {
            h = {a.x, b.x, c.x, d.x, a.y, b.y, c.y, d.y, a.z, b.z, c.z, d.z, a.w, b.w, c.w, d.w};
        }
    }

    template<typename T>
    mat4_t<T>::mat4_t(T d[16]) : data{d[0], d[1], d[2], d[3], d[4], d[5], d[6], d[7], d[9], d[10], d[11], d[12], d[13], d[14], d[15]} {}

    template<typename T>
    mat4_t<T> mat4_t<T>::RotationBasic(T angle, Plane plane) {
        T s = sin(angle), c = cos(angle);
        switch(plane) {
            case Plane::XW: {
                return mat4_t<T>(
                    T(1), T(0), T(0), T(0),
                    T(0), c,    -s,   T(0),
                    T(0), s,    c,    T(0),
                    T(0), T(0), T(0), T(1)
                );
            }
            case Plane::YW: {
                return mat4_t<T>(
                    c,    T(0), s,    T(0),
                    T(0), T(1), T(0), T(0),
                    -s,   T(0), c,    T(0),
                    T(0), T(0), T(0), T(1)
                );
            }
            case Plane::ZW: {
                return mat4_t<T>(
                    c,    -s,   T(0), T(0),
                    s,    c,    T(0), T(0),
                    T(0), T(0), T(1), T(0),
                    T(0), T(0), T(0), T(1)
                );
            }
            case Plane::XY: {
                return mat4_t<T>(
                    T(1), T(0), T(0), T(0),
                    T(0), T(1), T(0), T(0),
                    T(0), T(0), c,    -s,
                    T(0), T(0), s,    c
                );
            }
            case Plane::YZ: {
                return mat4_t<T>(
                    c,    T(0), T(0), -s,
                    T(0), T(1), T(0), T(0),
                    T(0), T(0), T(1), T(0),
                    s,    T(0), T(0), c
                );
            }
            case Plane::ZX: {
                return mat4_t<T>(
                    T(1), T(0), T(0), T(0),
                    T(0), c,    T(0), s,
                    T(0), T(0), T(1), T(0),
                    T(0), -s,   T(0), c
                );
            }
        }
        return mat4_t<T>();
    }

    template<typename T>
    mat4_t<T> mat4_t<T>::RotationBasic(T angle, Axis axis) {
        switch(axis) {
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

    template<typename T>
    mat4_t<T> mat4_t<T>::Rotation(T angle, vec3_t<T> axis) {
        T s = sin(angle), c = cos(angle);
        T ic = 1-c;
        vec3_t<T> a = normalize(axis);
        T xx = square(a.x), yy = square(a.y), zz = square(a.z),
            xy = a.x*a.y,     xz = a.x*a.z,     yz = a.y*a.z;
        return mat4_t<T>(
            c + xx*ic,          xy*ic - a.z*s,      xz*ic + a.y*s,  T(0),
            xy*ic + a.z*s,      c + yy*ic,          yz*ic - a.x*s,  T(0),
            xz*ic - a.y*s,      yz*ic + a.x*s,      c + zz*ic,      T(0),
            T(0),               T(0),               T(0),           T(1)
        );
    }

    template<typename T>
    mat4_t<T> mat4_t<T>::Scaler(vec4_t<T> scale) {
        return mat4_t<T>(scale.x, T(0), T(0), T(0), T(0), scale.y, T(0), T(0), T(0), T(0), scale.z, T(0), T(0), T(0), T(0), scale.w);
    }

#endif // MATH_MAT4

#ifdef MATH_COMPLEX
    template<typename T>
    complex_t<T>::complex_t() : x(0) , y(0) {}

    template<typename T>
    complex_t<T>::complex_t(T a) : x(a) , y(0) {}

    template<typename T>
    complex_t<T>::complex_t(T a, T b) : x(a) , y(b) {}

    template<typename T>
    complex_t<T>::complex_t(vec2_t<T> vec) : vector(vec) {}

    template<typename T>
    complex_t<T>::complex_t(T d[2]) : x(d[0]) , y(d[1]) {}

    template<typename T>
    complex_t<T> exp(const complex_t<T>& a) {
        return complex_t<T>(cos(a.imag), sin(a.imag)) * exp(a.real);
    }

    #ifdef MATH_F32
        template complex_t<f32> exp(const complex_t<f32>&);
    #endif
    #ifdef MATH_F64
        template complex_t<f64> exp(const complex_t<f64>&);
    #endif
#endif // MATH_COMPLEX

#ifdef MATH_QUATERNION
    template<typename T>
    quat_t<T>::quat_t() : data{1, 0, 0, 0} {}

    template<typename T>
    quat_t<T>::quat_t(T a) : data{a, 0, 0, 0} {}

    template<typename T>
    quat_t<T>::quat_t(T a, vec3_t<T> v) : scalar(a), vector(v) {}

    template<typename T>
    quat_t<T>::quat_t(vec4_t<T> v) : wxyz(v) {}

    template<typename T>
    quat_t<T>::quat_t(T a, T b, T c, T d) : w(a), x(b), y(c), z(d) {}

    template<typename T>
    quat_t<T>::quat_t(T d[4]) : data{d[0], d[1], d[2], d[3]} {}

    template<typename T>
    vec3_t<T> quat_t<T>::RotatePoint(vec3_t<T> point, T angle, vec3_t<T> axis) {
        quat_t<T> rot = Rotation(angle, axis);
        rot = rot * quat_t<T>(T(0), point) * rot.Conjugate();
        return rot.vector;
    }

    template<typename T>
    vec3_t<T> quat_t<T>::RotatePoint(vec3_t<T> point) const {
        return ((*this) * quat_t<T>(T(0), point) * Conjugate()).vector;
    }

    template<typename T>
    quat_t<T> quat_t<T>::Rotate(T angle, vec3_t<T> axis) const {
        quat_t<T> rot = Rotation(angle, axis);
        return rot * (*this) * rot.Conjugate();
    }

    template<typename T>
    quat_t<T> quat_t<T>::Rotate(quat_t<T> rotation) const {
        return rotation * (*this) * rotation.Conjugate();
    }

    template<typename T>
    mat3_t<T> quat_t<T>::ToMat3() const {
        const T ir = w*x, jr = w*y, kr = w*z,
                ii = x*x, ij = x*y, ik = x*z,
                jj = y*y, jk = y*z,
                kk = z*z;
        return mat3_t<T>(
            1-2*(jj+kk),      2*(ij-kr),      2*(ik+jr),
              2*(ij+kr),    1-2*(ii+kk),      2*(jk-ir),
              2*(ik-jr),      2*(jk+ir),    1-2*(ii+jj)
        );
    }

    template<typename T>
    quat_t<T> slerp(quat_t<T> a, quat_t<T> b, T factor) {
        a = normalize(a);
        b = normalize(b);
        T d = dot(a.vector, b.vector);
        if (d < 0.0) {
            b = -b.wxyz;
            d *= -1.0;
        }
        const T threshold = 0.999;
        if (d > threshold) {
            return normalize(a + (b-a)*factor);
        }
        T thetaMax = acos(d);
        T theta = thetaMax*factor;
        T base = sin(theta) / sin(thetaMax);
        return a*(cos(theta) - d*base) + b*base;
    }

    #ifdef MATH_F32
        template quat_t<f32> slerp(quat_t<f32>, quat_t<f32>, f32);
    #endif
    #ifdef MATH_F64
        template quat_t<f64> slerp(quat_t<f64>, quat_t<f64>, f64);
    #endif
#endif // MATH_QUATERNION

// Make sure the templates we want are implemented.

#ifdef MATH_VEC2
    #ifdef MATH_F32
        template struct vec2_t<f32>;
    #endif
    #ifdef MATH_F64
        template struct vec2_t<f64>;
    #endif
    template struct vec2_t<i32>;
#endif
#ifdef MATH_VEC3
    #ifdef MATH_F32
        template struct vec3_t<f32>;
    #endif
    #ifdef MATH_F64
        template struct vec3_t<f64>;
    #endif
    template struct vec3_t<i32>;
#endif
#ifdef MATH_VEC4
    #ifdef MATH_F32
        template struct vec4_t<f32>;
    #endif
    #ifdef MATH_F64
        template struct vec4_t<f64>;
    #endif
    template struct vec4_t<i32>;
#endif
#ifdef MATH_MAT2
    #ifdef MATH_F32
        template struct mat2_t<f32>;
    #endif
    #ifdef MATH_F64
        template struct mat2_t<f64>;
    #endif
#endif
#ifdef MATH_MAT3
    #ifdef MATH_F32
        template struct mat3_t<f32>;
    #endif
    #ifdef MATH_F64
        template struct mat3_t<f64>;
    #endif
#endif
#ifdef MATH_MAT4
    #ifdef MATH_F32
        template struct mat4_t<f32>;
    #endif
    #ifdef MATH_F64
        template struct mat4_t<f64>;
    #endif
#endif
#ifdef MATH_COMPLEX
    #ifdef MATH_F32
        template struct complex_t<f32>;
    #endif
    #ifdef MATH_F64
        template struct complex_t<f64>;
    #endif
#endif
#ifdef MATH_QUATERNION
    #ifdef MATH_F32
        template struct quat_t<f32>;
    #endif
    #ifdef MATH_F64
        template struct quat_t<f64>;
    #endif
#endif
