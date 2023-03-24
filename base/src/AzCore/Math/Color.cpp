/*
	File: Color.cpp
	Author: Philip Haynes
*/

#include "Color.hpp"
#include "../math.hpp"
#include "../SIMD/SimdSSE2.hpp"

namespace AzCore {

vec3 lessThan(vec3 lhs, vec3 rhs) {
	f32x4 lhsV(lhs.r, lhs.g, lhs.b, 0.0f);
	f32x4 rhsV(rhs.r, rhs.g, rhs.b, 0.0f);
	u32x4 val = lhsV < rhsV;
	val &= u32x4(1);
	f32x4 resultV = f32x4(i32x4(val));
	vec4 result;
	resultV.GetValues(result.data);
	return result.rgb;
}
vec3 pow(vec3 a, f32 b) {
	// TODO: Use an approximation and SIMD
	vec3 result;
	result.x = ::pow(a.x, b);
	result.y = ::pow(a.y, b);
	result.z = ::pow(a.z, b);
	return result;
}
vec3 mix(vec3 a, vec3 b, vec3 t) {
	return a + (b - a) * t;
}
f32 mix(f32 a, f32 b, f32 t) {
	return a + (b - a) * t;
}

vec3 sRGBToLinear(vec3 in) {
	vec3 cutoff = lessThan(in, vec3(0.04045f));
	vec3 higher = pow((in + vec3(0.055f))/vec3(1.055f), 2.4f);
	vec3 lower = in/vec3(12.92f);
	return mix(higher, lower, cutoff);
}

vec3 linearTosRGB(vec3 in) {
	vec3 cutoff = lessThan(in, vec3(0.0031308f));
	vec3 higher = pow(in, 1.0f/2.4f)*vec3(1.055f)-vec3(0.055f);
	vec3 lower = in*vec3(12.92f);
	return mix(higher, lower, cutoff);
}

f32 sRGBToLinear(f32 in) {
	f32 cutoff = in < 0.04045f;
	f32 higher = ::pow((in + 0.055f)/1.055f, 2.4f);
	f32 lower = in/12.92f;
	return mix(higher, lower, cutoff);
}

f32 linearTosRGB(f32 in) {
	f32 cutoff = in < 0.0031308f;
	f32 higher = ::pow(in, 1.0f/2.4f)*1.055f-0.055f;
	f32 lower = in*12.92f;
	return mix(higher, lower, cutoff);
}

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
	hsv.v = max(rgb.r, rgb.g, rgb.b);
	if (hsv.v == T(0.0))
		return hsv; // Black can't encode saturation or hue
	rgb /= hsv.v;
	hsv.s = T(1.0) - min(rgb.r, rgb.g, rgb.b);
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

template vec3_t<f32> hsvToRgb(vec3_t<f32>);
template vec3_t<f32> rgbToHsv(vec3_t<f32>);
template vec3_t<f64> hsvToRgb(vec3_t<f64>);
template vec3_t<f64> rgbToHsv(vec3_t<f64>);

} // namespace AzCore