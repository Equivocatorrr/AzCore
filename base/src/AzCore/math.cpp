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

f32 random(f32 min, f32 max, RandomNumberGenerator& rng) {
    u32 num = rng.Generate();
    return ((f64)num * (f64)(max-min) / (f64)UINT32_MAX) + (f64)min;
}

i32 random(i32 min, i32 max, RandomNumberGenerator& rng) {
    return i32(rng.Generate() % (max - min)) + min;
}

#ifdef MATH_F32
    Radians32 angleDiff(Angle32 from, Angle32 to) {
        Radians32 diff = Radians32(to) - Radians32(from);
        while (diff >= pi) {
            diff -= tau;
        }
        while (diff < -pi) {
            diff += tau;
        }
        return diff;
    }
#endif
#ifdef MATH_F64
    Radians64 angleDiff(Angle64 from, Angle64 to) {
        Radians64 diff = Radians64(to) - Radians64(from);
        while (diff >= pi64) {
            diff -= tau64;
        }
        while (diff < -pi64) {
            diff += tau64;
        }
        return diff;
    }
#endif

// Template stuff is pretty ugly, but it saves a lot of lines.
#ifdef MATH_VEC2
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
    vec3_t<T> vec3_t<T>::operator+=(const vec3_t<T>& a) {
        x += a.x;
        y += a.y;
        z += a.z;
        return *this;
    }

    template<typename T>
    vec3_t<T> vec3_t<T>::operator-=(const vec3_t<T>& a) {
        x -= a.x;
        y -= a.y;
        z -= a.z;
        return *this;
    }

    template<typename T>
    vec3_t<T> vec3_t<T>::operator/=(const vec3_t<T>& a) {
        x /= a.x;
        y /= a.y;
        z /= a.z;
        return *this;
    }

    template<typename T>
    vec3_t<T> vec3_t<T>::operator/=(const T& a) {
        x /= a;
        y /= a;
        z /= a;
        return *this;
    }

    template<typename T>
    vec3_t<T> vec3_t<T>::operator*=(const vec3_t<T>& a) {
        x *= a.x;
        y *= a.y;
        z *= a.z;
        return *this;
    }

    template<typename T>
    vec3_t<T> vec3_t<T>::operator*=(const T& a) {
        x *= a;
        y *= a;
        z *= a;
        return *this;
    }

    template<typename T>
    vec3_t<T> hsvToRgb(vec3_t<T> hsv) {
        vec3_t<T> rgb(0.0);
        i32 section = i32(hsv.h*6.0);
        T fraction = hsv.h*6.0 - T(section);
        section = section%6;
        switch (section) {
            case 0: {
                rgb.r = 1.0;
                rgb.g = fraction;
                break;
            }
            case 1: {
                rgb.r = 1.0-fraction;
                rgb.g = 1.0;
                break;
            }
            case 2: {
                rgb.g = 1.0;
                rgb.b = fraction;
                break;
            }
            case 3: {
                rgb.g = 1.0-fraction;
                rgb.b = 1.0;
                break;
            }
            case 4: {
                rgb.b = 1.0;
                rgb.r = fraction;
                break;
            }
            case 5: {
                rgb.b = 1.0-fraction;
                rgb.r = 1.0;
                break;
            }
        }
        // We now have the RGB of the hue at 100% saturation and value
        // To reduce saturation just blend the whole thing with white
        rgb = lerp(vec3_t<T>(1.0), rgb, hsv.s);
        // To reduce value just blend the whole thing with black
        rgb *= hsv.v;
        return rgb;
    }

    template<typename T>
    vec3_t<T> rgbToHsv(vec3_t<T> rgb) {
        vec3_t<T> hsv(0.0);
        hsv.v = max(max(rgb.r, rgb.g), rgb.b);
        if (hsv.v == 0.0)
            return hsv; // Black can't encode saturation or hue
        rgb /= hsv.v;
        hsv.s = 1.0 - min(min(rgb.r, rgb.g), rgb.b);
        if (hsv.s == 0.0)
            return hsv; // Grey can't encode hue
        rgb = map(rgb, vec3_t<T>(1.0-hsv.s), vec3_t<T>(1.0), vec3_t<T>(0.0), vec3_t<T>(1.0));
        if (rgb.r >= rgb.g && rgb.g > rgb.b) {
            hsv.h = 0.0+rgb.g;
        } else if (rgb.g >= rgb.r && rgb.r > rgb.b) {
            hsv.h = 2.0-rgb.r;
        } else if (rgb.g >= rgb.b && rgb.b > rgb.r) {
            hsv.h = 2.0+rgb.b;
        } else if (rgb.b >= rgb.g && rgb.g > rgb.r) {
            hsv.h = 4.0-rgb.g;
        } else if (rgb.b >= rgb.r && rgb.r > rgb.g) {
            hsv.h = 4.0+rgb.r;
        } else if (rgb.r >= rgb.b && rgb.b > rgb.g) {
            hsv.h = 6.0-rgb.b;
        }
        hsv.h /= 6.0;
        return hsv;
    }

    #ifdef MATH_F32
        template vec3_t<f32> hsvToRgb(vec3_t<f32>);
        template vec3_t<f32> rgbToHsv(vec3_t<f32>);
    #endif
    #ifdef MATH_F64
        template vec3_t<f64> hsvToRgb(vec3_t<f64>);
        template vec3_t<f64> rgbToHsv(vec3_t<f64>);
    #endif

#endif // MATH_VEC3

#ifdef MATH_MAT3

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
    vec4_t<T> vec4_t<T>::operator+=(const vec4_t<T>& vec) {
        x += vec.x;
        y += vec.y;
        z += vec.z;
        w += vec.w;
        return *this;
    }

    template<typename T>
    vec4_t<T> vec4_t<T>::operator-=(const vec4_t<T>& vec) {
        x -= vec.x;
        y -= vec.y;
        z -= vec.z;
        w -= vec.w;
        return *this;
    }

    template<typename T>
    vec4_t<T> vec4_t<T>::operator/=(const vec4_t<T>& vec) {
        x /= vec.x;
        y /= vec.y;
        z /= vec.z;
        w /= vec.w;
        return *this;
    }

    template<typename T>
    vec4_t<T> vec4_t<T>::operator/=(const T& vec) {
        x /= vec;
        y /= vec;
        z /= vec;
        w /= vec;
        return *this;
    }

    template<typename T>
    vec4_t<T> vec4_t<T>::operator*=(const vec4_t<T>& vec) {
        x *= vec.x;
        y *= vec.y;
        z *= vec.z;
        w *= vec.w;
        return *this;
    }

    template<typename T>
    vec4_t<T> vec4_t<T>::operator*=(const T& vec) {
        x *= vec;
        y *= vec;
        z *= vec;
        w *= vec;
        return *this;
    }

#endif // MATH_VEC4

#ifdef MATH_MAT4

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

#ifdef MATH_VEC5

    template<typename T>
    vec5_t<T> vec5_t<T>::operator+=(const vec5_t<T>& vec) {
        x += vec.x;
        y += vec.y;
        z += vec.z;
        w += vec.w;
        v += vec.v;
        return *this;
    }

    template<typename T>
    vec5_t<T> vec5_t<T>::operator-=(const vec5_t<T>& vec) {
        x -= vec.x;
        y -= vec.y;
        z -= vec.z;
        w -= vec.w;
        v -= vec.v;
        return *this;
    }

    template<typename T>
    vec5_t<T> vec5_t<T>::operator/=(const vec5_t<T>& vec) {
        x /= vec.x;
        y /= vec.y;
        z /= vec.z;
        w /= vec.w;
        v /= vec.v;
        return *this;
    }

    template<typename T>
    vec5_t<T> vec5_t<T>::operator/=(const T& vec) {
        x /= vec;
        y /= vec;
        z /= vec;
        w /= vec;
        v /= vec;
        return *this;
    }

    template<typename T>
    vec5_t<T> vec5_t<T>::operator*=(const vec5_t<T>& vec) {
        x *= vec.x;
        y *= vec.y;
        z *= vec.z;
        w *= vec.w;
        v *= vec.v;
        return *this;
    }

    template<typename T>
    vec5_t<T> vec5_t<T>::operator*=(const T& vec) {
        x *= vec;
        y *= vec;
        z *= vec;
        w *= vec;
        v *= vec;
        return *this;
    }

#endif // MATH_VEC5

#ifdef MATH_MAT5

    template<typename T>
    mat5_t<T> mat5_t<T>::RotationBasic(T angle, Plane plane) {
        T s = sin(angle), c = cos(angle);
        switch(plane) {
            case Plane::XW: {
                return mat5_t<T>(
                    T(1), T(0), T(0), T(0), T(0),
                    T(0), c,    -s,   T(0), T(0),
                    T(0), s,    c,    T(0), T(0),
                    T(0), T(0), T(0), T(1), T(0),
                    T(0), T(0), T(0), T(0), T(1)
                );
            }
            case Plane::YW: {
                return mat5_t<T>(
                    c,    T(0), s,    T(0), T(0),
                    T(0), T(1), T(0), T(0), T(0),
                    -s,   T(0), c,    T(0), T(0),
                    T(0), T(0), T(0), T(1), T(0),
                    T(0), T(0), T(0), T(0), T(1)
                );
            }
            case Plane::ZW: {
                return mat5_t<T>(
                    c,    -s,   T(0), T(0), T(0),
                    s,    c,    T(0), T(0), T(0),
                    T(0), T(0), T(1), T(0), T(0),
                    T(0), T(0), T(0), T(1), T(0),
                    T(0), T(0), T(0), T(0), T(1)
                );
            }
            case Plane::XY: {
                return mat5_t<T>(
                    T(1), T(0), T(0), T(0), T(0),
                    T(0), T(1), T(0), T(0), T(0),
                    T(0), T(0), c,    -s,   T(0),
                    T(0), T(0), s,    c,    T(0),
                    T(0), T(0), T(0), T(0), T(1)
                );
            }
            case Plane::YZ: {
                return mat5_t<T>(
                    c,    T(0), T(0), -s,   T(0),
                    T(0), T(1), T(0), T(0), T(0),
                    T(0), T(0), T(1), T(0), T(0),
                    s,    T(0), T(0), c,    T(0),
                    T(0), T(0), T(0), T(0), T(1)
                );
            }
            case Plane::ZX: {
                return mat5_t<T>(
                    T(1), T(0), T(0), T(0), T(0),
                    T(0), c,    T(0), s,    T(0),
                    T(0), T(0), T(1), T(0), T(0),
                    T(0), -s,   T(0), c,    T(0),
                    T(0), T(0), T(0), T(0), T(1)
                );
            }
        }
        return mat5_t<T>();
    }

    template<typename T>
    mat5_t<T> mat5_t<T>::RotationBasic(T angle, Axis axis) {
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
        return mat5_t<T>();
    }

    template<typename T>
    mat5_t<T> mat5_t<T>::Rotation(T angle, vec3_t<T> axis) {
        T s = sin(angle), c = cos(angle);
        T ic = 1-c;
        vec3_t<T> a = normalize(axis);
        T xx = square(a.x), yy = square(a.y), zz = square(a.z),
            xy = a.x*a.y,     xz = a.x*a.z,     yz = a.y*a.z;
        return mat5_t<T>(
            c + xx*ic,          xy*ic - a.z*s,      xz*ic + a.y*s,  T(0),  T(0),
            xy*ic + a.z*s,      c + yy*ic,          yz*ic - a.x*s,  T(0),  T(0),
            xz*ic - a.y*s,      yz*ic + a.x*s,      c + zz*ic,      T(0),  T(0),
            T(0),               T(0),               T(0),           T(1),  T(0),
            T(0),               T(0),               T(0),           T(0),  T(1)
        );
    }

    template<typename T>
    mat5_t<T> mat5_t<T>::Scaler(vec5_t<T> scale) {
        return mat5_t<T>(scale.x, T(0), T(0), T(0), T(0), T(0), scale.y, T(0), T(0), T(0), T(0), T(0), scale.z, T(0), T(0), T(0), T(0), T(0), scale.w, T(0), T(0), T(0), T(0), T(0), scale.v);
    }

#endif // MATH_MAT5

#ifdef MATH_COMPLEX

    template<typename T>
    complex_t<T>& complex_t<T>::operator+=(const complex_t<T>& a) {
        *this = *this + a;
        return *this;
    }

    template<typename T>
    complex_t<T>& complex_t<T>::operator-=(const complex_t<T>& a) {
        *this = *this - a;
        return *this;
    }

    template<typename T>
    complex_t<T>& complex_t<T>::operator*=(const complex_t<T>& a) {
        *this = *this * a;
        return *this;
    }

    template<typename T>
    complex_t<T>& complex_t<T>::operator/=(const complex_t<T>& a) {
        *this = *this / a;
        return *this;
    }

    template<typename T>
    complex_t<T>& complex_t<T>::operator+=(const T& a) {
        *this = *this + a;
        return *this;
    }

    template<typename T>
    complex_t<T>& complex_t<T>::operator-=(const T& a) {
        *this = *this - a;
        return *this;
    }

    template<typename T>
    complex_t<T>& complex_t<T>::operator*=(const T& a) {
        *this = *this * a;
        return *this;
    }

    template<typename T>
    complex_t<T>& complex_t<T>::operator/=(const T& a) {
        *this = *this / a;
        return *this;
    }

    template<typename T>
    complex_t<T> exp(const complex_t<T>& a) {
        return complex_t<T>(cos(a.imag), sin(a.imag)) * exp(a.real);
    }

    template<typename T>
    complex_t<T> log(const complex_t<T>& a) {
        return complex_t<T>(log(abs(a)), atan2(a.imag, a.real));
    }

    template<typename T>
    complex_t<T> pow(const complex_t<T>& a, const complex_t<T>& e) {
        return exp(log(a)*e);
    }

    template<typename T>
    complex_t<T> pow(const complex_t<T>& a, const T& e) {
        return exp(log(a)*e);
    }

    #ifdef MATH_F32
        template complex_t<f32> exp(const complex_t<f32>&);
        template complex_t<f32> log(const complex_t<f32>&);
        template complex_t<f32> pow(const complex_t<f32>&,const complex_t<f32>&);
        template complex_t<f32> pow(const complex_t<f32>&,const f32&);
    #endif
    #ifdef MATH_F64
        template complex_t<f64> exp(const complex_t<f64>&);
        template complex_t<f64> log(const complex_t<f64>&);
        template complex_t<f64> pow(const complex_t<f64>&,const complex_t<f64>&);
        template complex_t<f64> pow(const complex_t<f64>&,const f64&);
    #endif
#endif // MATH_COMPLEX

#ifdef MATH_QUATERNION

    template<typename T>
    quat_t<T>& quat_t<T>::operator+=(const quat_t<T>& a) {
        *this = *this + a;
        return *this;
    }

    template<typename T>
    quat_t<T>& quat_t<T>::operator-=(const quat_t<T>& a) {
        *this = *this - a;
        return *this;
    }

    template<typename T>
    quat_t<T>& quat_t<T>::operator*=(const quat_t<T>& a) {
        *this = *this * a;
        return *this;
    }

    template<typename T>
    quat_t<T>& quat_t<T>::operator/=(const quat_t<T>& a) {
        *this = *this / a;
        return *this;
    }

    template<typename T>
    quat_t<T>& quat_t<T>::operator+=(const T& a) {
        *this = *this + a;
        return *this;
    }

    template<typename T>
    quat_t<T>& quat_t<T>::operator-=(const T& a) {
        *this = *this - a;
        return *this;
    }

    template<typename T>
    quat_t<T>& quat_t<T>::operator*=(const T& a) {
        *this = *this * a;
        return *this;
    }

    template<typename T>
    quat_t<T>& quat_t<T>::operator/=(const T& a) {
        *this = *this / a;
        return *this;
    }

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

    template<typename T>
    quat_t<T> exp(quat_t<T> a) {
        T theta = abs(a.vector);
        return quat_t<T>(cos(theta), theta > 0.0000001 ? (a.vector * sin(theta) / theta) : vec3_t<T>(0)) * exp(a.scalar);
    }

    template<typename T>
    quat_t<T> log(quat_t<T> a) {
        // if (a.scalar < 0)
        //     a *= -1;
        T len = log(a.Norm());
        T vLen = abs(a.vector);
        T theta = atan2(vLen, a.scalar);
        return quat_t<T>(len, vLen > 0.0000001 ? (a.vector / vLen * theta) : vec3_t<T>(theta, 0, 0));
    }

    template<typename T>
    quat_t<T> pow(const quat_t<T>& a, const quat_t<T>& e) {
        return exp(log(a)*e);
    }

    template<typename T>
    quat_t<T> pow(const quat_t<T>& a, const T& e) {
        return exp(log(a)*e);
    }

    #ifdef MATH_F32
        template quat_t<f32> slerp(quat_t<f32>, quat_t<f32>, f32);
        template quat_t<f32> exp(quat_t<f32>);
        template quat_t<f32> log(quat_t<f32>);
        template quat_t<f32> pow(const quat_t<f32>&, const quat_t<f32>&);
        template quat_t<f32> pow(const quat_t<f32>&, const f32&);
    #endif
    #ifdef MATH_F64
        template quat_t<f64> slerp(quat_t<f64>, quat_t<f64>, f64);
        template quat_t<f64> exp(quat_t<f64>);
        template quat_t<f64> log(quat_t<f64>);
        template quat_t<f64> pow(const quat_t<f64>&, const f64&);
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
#ifdef MATH_VEC5
    #ifdef MATH_F32
        template struct vec5_t<f32>;
    #endif
    #ifdef MATH_F64
        template struct vec5_t<f64>;
    #endif
    template struct vec5_t<i32>;
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
#ifdef MATH_MAT5
    #ifdef MATH_F32
        template struct mat5_t<f32>;
    #endif
    #ifdef MATH_F64
        template struct mat5_t<f64>;
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
