/*
	File: mat4_t.cpp
	Author: Philip Haynes
*/

#include "mat4_t.hpp"

namespace AzCore {

template <typename T>
mat4_t<T> mat4_t<T>::Camera(vec3_t<T> pos, vec3_t<T> forward, vec3_t<T> up) {
	vec3_t<T> right = normalize(cross(forward, up));
	// Orthogonalize since it's easier if the up vector can be fixed
	up = orthogonalize(up, forward);
	vec3_t<T> offset = vec3_t<T>(
		-dot(right,   pos),
		-dot(forward, pos),
		-dot(up,      pos)
	);
	mat4_t<T> result = FromRows(
		vec4_t<T>(right,   offset.x),
		vec4_t<T>(forward, offset.y),
		vec4_t<T>(up,      offset.z),
		vec4_t<T>(vec3_t<T>(0),  T(1))
	);
	return result;
}

template <typename T>
mat4_t<T> mat4_t<T>::Perspective(Radians<T> fovX, T widthOverHeight, T nearClip, T farClip) {
	AzAssert(widthOverHeight != T(0), "Invalid aspect ratio");
	AzAssert(nearClip < farClip, "Invalid clipping planes");
	AzAssert(fovX.value() > 0.0f, "Invalid field of view");
	
	T fovFac = T(1) / tan(fovX * T(0.5));
	
	T x =  fovFac;
	T y = -fovFac * widthOverHeight;
	T a =  farClip / (farClip - nearClip);
	T b = -nearClip * a;
	
	mat4_t<T> result = mat4_t<T>::FromRows(
		vec4_t<T>(x   , T(0), T(0), T(0)),
		vec4_t<T>(T(0), T(0), y   , T(0)),
		vec4_t<T>(T(0), a   , T(0), b   ),
		vec4_t<T>(T(0), T(1), T(0), T(0))
	);
	return result;
}

template <typename T>
mat4_t<T> mat4_t<T>::Ortho(T width, T height, T nearClip, T farClip) {
	AzAssert(nearClip < farClip, "Invalid clipping planes");
	
	T x =  T(2) / width;
	T y = -T(2) / height;
	T a =  T(1) / (farClip - nearClip);
	T b =  nearClip * a;
	
	mat4_t<T> result = mat4_t<T>::FromRows(
		vec4_t<T>(x   , T(0), T(0), T(0)),
		vec4_t<T>(T(0), T(0), y   , T(0)),
		vec4_t<T>(T(0), a   , T(0), b   ),
		vec4_t<T>(T(0), T(0), T(0), T(1))
	);
	
	return result;
}

template mat4 mat4::Camera(vec3 pos, vec3 forward, vec3 up);
template mat4 mat4::Perspective(Radians<f32> fovX, f32 widthOverHeight, f32 nearClip, f32 farClip);
template mat4 mat4::Ortho(f32 width, f32 height, f32 nearClip, f32 farClip);
template mat4d mat4d::Camera(vec3d pos, vec3d forward, vec3d up);
template mat4d mat4d::Perspective(Radians<f64> fovX, f64 widthOverHeight, f64 nearClip, f64 farClip);
template mat4d mat4d::Ortho(f64 width, f64 height, f64 nearClip, f64 farClip);

} // namespace AzCore
