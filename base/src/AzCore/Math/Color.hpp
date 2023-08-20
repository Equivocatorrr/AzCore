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

constexpr vec4 ColorFromARGB(u32 code) {
	vec4 result = vec4();
	result.b = f32((code >> 8*0) & 0xFF) / 255.0f;
	result.g = f32((code >> 8*1) & 0xFF) / 255.0f;
	result.r = f32((code >> 8*2) & 0xFF) / 255.0f;
	result.a = f32((code >> 8*3) & 0xFF) / 255.0f;
	return result;
}

constexpr vec3 ColorFromRGB(u32 code) {
	vec3 result = vec3();
	result.b = f32((code >> 8*0) & 0xFF) / 255.0f;
	result.g = f32((code >> 8*1) & 0xFF) / 255.0f;
	result.r = f32((code >> 8*2) & 0xFF) / 255.0f;
	return result;
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