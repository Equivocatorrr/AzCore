/*
    File: math.cpp
    Author: Philip Haynes
*/

#include "math.hpp"

#include "Math/RandomNumberGenerator.cpp"
#include "Math/Angle.cpp"

namespace AzCore {

#ifdef AZCORE_MATH_VEC3

    template<typename T>
    vec3_t<T> hsvToRgb(vec3_t<T> hsv) {
        vec3_t<T> rgb(0.0);
        i32 section = i32(hsv.h*T(6.0));
        T fraction = hsv.h*T(6.0) - T(section);
        section = section%6;
        switch (section) {
            case 0: {
                rgb.r = T(1.0);
                rgb.g = fraction;
                break;
            }
            case 1: {
                rgb.r = T(1.0)-fraction;
                rgb.g = T(1.0);
                break;
            }
            case 2: {
                rgb.g = T(1.0);
                rgb.b = fraction;
                break;
            }
            case 3: {
                rgb.g = T(1.0)-fraction;
                rgb.b = T(1.0);
                break;
            }
            case 4: {
                rgb.b = T(1.0);
                rgb.r = fraction;
                break;
            }
            case 5: {
                rgb.b = T(1.0)-fraction;
                rgb.r = T(1.0);
                break;
            }
        }
        // We now have the RGB of the hue at 100% saturation and value
        // To reduce saturation just blend the whole thing with white
        rgb = lerp(vec3_t<T>(T(1.0)), rgb, hsv.s);
        // To reduce value just blend the whole thing with black
        rgb *= hsv.v;
        return rgb;
    }

    template<typename T>
    vec3_t<T> rgbToHsv(vec3_t<T> rgb) {
        vec3_t<T> hsv(T(0.0));
        hsv.v = max(max(rgb.r, rgb.g), rgb.b);
        if (hsv.v == T(0.0))
            return hsv; // Black can't encode saturation or hue
        rgb /= hsv.v;
        hsv.s = T(1.0) - min(min(rgb.r, rgb.g), rgb.b);
        if (hsv.s == T(0.0))
            return hsv; // Grey can't encode hue
        rgb = map(rgb, vec3_t<T>(T(1.0)-hsv.s), vec3_t<T>(T(1.0)), vec3_t<T>(T(0.0)), vec3_t<T>(T(1.0)));
        if (rgb.r >= rgb.g && rgb.g > rgb.b) {
            hsv.h = T(0.0)+rgb.g;
        } else if (rgb.g >= rgb.r && rgb.r > rgb.b) {
            hsv.h = T(2.0)-rgb.r;
        } else if (rgb.g >= rgb.b && rgb.b > rgb.r) {
            hsv.h = T(2.0)+rgb.b;
        } else if (rgb.b >= rgb.g && rgb.g > rgb.r) {
            hsv.h = T(4.0)-rgb.g;
        } else if (rgb.b >= rgb.r && rgb.r > rgb.g) {
            hsv.h = T(4.0)+rgb.r;
        } else if (rgb.r >= rgb.b && rgb.b > rgb.g) {
            hsv.h = T(6.0)-rgb.b;
        }
        hsv.h /= T(6.0);
        return hsv;
    }

    #ifdef AZCORE_MATH_F32
        template vec3_t<f32> hsvToRgb(vec3_t<f32>);
        template vec3_t<f32> rgbToHsv(vec3_t<f32>);
    #endif
    #ifdef AZCORE_MATH_F64
        template vec3_t<f64> hsvToRgb(vec3_t<f64>);
        template vec3_t<f64> rgbToHsv(vec3_t<f64>);
    #endif

#endif // AZCORE_MATH_VEC3

} // namespace AzCore
