/*
	File: Color.hpp
	Author: Philip Haynes
	Utilities for working with colors and color spaces.
*/

#ifndef AZCORE_COLOR_HPP
#define AZCORE_COLOR_HPP

#include "vec3_t.hpp"
#include "vec4_t.hpp"

namespace AzCore {

template <typename T>
using Color = vec4_t<T>;

inline Color<u8> FloatTo8Bit(Color<f32> in) {
	return in*255.0f;
}

vec3 sRGBToLinear(vec3 in);
vec3 linearTosRGB(vec3 in);

inline Color<f32> sRGBToLinear(Color<f32> in) {
	return Color<f32>(sRGBToLinear(in.rgb), in.a);
}

inline Color<f32> linearTosRGB(Color<f32> in) {
	return Color<f32>(linearTosRGB(in.rgb), in.a);
}

f32 sRGBToLinear(f32 in);
f32 linearTosRGB(f32 in);

template <typename T>
vec3_t<T> hsvToRgb(vec3_t<T> hsv);

template <typename T>
vec3_t<T> rgbToHsv(vec3_t<T> rgb);

} // namespace AzCore

#endif // AZCORE_COLOR_HPP