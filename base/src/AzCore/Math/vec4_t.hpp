/*
	File: vec4_t.hpp
	Author: Philip Haynes
*/

#ifndef AZCORE_MATH_VEC4_HPP
#define AZCORE_MATH_VEC4_HPP

#include "vec3_t.hpp"

#include "basic.hpp"

namespace AzCore {

template <typename T>
struct vec4_t {
	union {
		struct {
			T x, y, z, w;
		};
		struct {
			T r, g, b, a;
		};
		struct {
			T h, s, v;
		};
		struct {
			vec2_t<T> xy, zw;
		};
		struct {
			T __x; vec2_t<T> yz;
		};
		struct {
			vec2_t<T> rg, ba;
		};
		struct {
			T __r; vec2_t<T> gb;
		};
		struct {
			vec3_t<T> xyz;
		};
		struct {
			T ___x; vec3_t<T> yzw;
		};
		struct {
			vec3_t<T> rgb;
		};
		struct {
			T ___r; vec3_t<T> gba;
		};
		struct {
			vec3_t<T> hsv;
		};
		struct {
			T data[4];
		};
	};

	vec4_t() = default;
	inline vec4_t(T a) : x(a), y(a), z(a), w(a) {}
	inline vec4_t(T _x, T _y, T _z, T _w) : x(_x), y(_y), z(_z), w(_w) {}
	inline vec4_t(vec3_t<T> _xyz, T _w) : x(_xyz.x), y(_xyz.y), z(_xyz.z), w(_w) {}
	inline vec4_t(T _x, vec3_t<T> _yzw) : x(_x), y(_yzw.x), z(_yzw.y), w(_yzw.z) {}
	inline vec4_t(vec2_t<T> _xy, vec2_t<T> _zw) : xy(_xy), zw(_zw) {}
	inline vec4_t(vec2_t<T> _xy, T _z, T _w) : x(_xy.x), y(_xy.y), z(_z), w(_w) {}
	inline vec4_t(T _x, vec2_t<T> _yz, T _w) : x(_x), y(_yz.x), z(_yz.y), w(_w) {}
	inline vec4_t(T _x, T _y, vec2_t<T> _zw) : x(_x), y(_y), z(_zw.x), w(_zw.y) {}
	template <typename I>
	vec4_t(vec4_t<I> a) : x((T)a.x), y((T)a.y), z((T)a.z), w((T)a.w) {}
	inline vec4_t<T> operator+(vec4_t<T> vec) const { return vec4_t<T>(x + vec.x, y + vec.y, z + vec.z, w + vec.w); }
	inline vec4_t<T> operator-(vec4_t<T> vec) const { return vec4_t<T>(x - vec.x, y - vec.y, z - vec.z, w - vec.w); }
	inline vec4_t<T> operator-() const { return vec4_t<T>(-x, -y, -z, -w); }
	inline vec4_t<T> operator*(vec4_t<T> vec) const { return vec4_t<T>(x * vec.x, y * vec.y, z * vec.z, w * vec.w); }
	inline vec4_t<T> operator/(vec4_t<T> vec) const { return vec4_t<T>(x / vec.x, y / vec.y, z / vec.z, w / vec.w); }
	inline vec4_t<T> operator*(T vec) const { return vec4_t<T>(x * vec, y * vec, z * vec, w * vec); }
	inline vec4_t<T> operator/(T vec) const { return vec4_t<T>(x / vec, y / vec, z / vec, w / vec); }
	inline bool operator==(vec4_t<T> a) const { return x == a.x && y == a.y && z == a.z && w == a.w; }
	inline bool operator!=(vec4_t<T> a) const { return x != a.x || y != a.y || z != a.z || w != a.w; }
	inline T &operator[](u32 i) { return data[i]; }
	inline vec4_t<T> operator+=(vec4_t<T> vec) {
		x += vec.x;
		y += vec.y;
		z += vec.z;
		w += vec.w;
		return *this;
	}
	inline vec4_t<T> operator-=(vec4_t<T> vec) {
		x -= vec.x;
		y -= vec.y;
		z -= vec.z;
		w -= vec.w;
		return *this;
	}
	inline vec4_t<T> operator/=(vec4_t<T> vec) {
		x /= vec.x;
		y /= vec.y;
		z /= vec.z;
		w /= vec.w;
		return *this;
	}
	inline vec4_t<T> operator/=(T vec) {
		x /= vec;
		y /= vec;
		z /= vec;
		w /= vec;
		return *this;
	}
	inline vec4_t<T> operator*=(vec4_t<T> vec) {
		x *= vec.x;
		y *= vec.y;
		z *= vec.z;
		w *= vec.w;
		return *this;
	}
	inline vec4_t<T> operator*=(T vec) {
		x *= vec;
		y *= vec;
		z *= vec;
		w *= vec;
		return *this;
	}
};

} // namespace AzCore

template <typename T>
inline AzCore::vec4_t<T> operator*(T a, AzCore::vec4_t<T> b) {
	return b * a;
}

template <typename T>
inline T dot(AzCore::vec4_t<T> a, AzCore::vec4_t<T> b) {
	return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

template <typename T>
inline T normSqr(AzCore::vec4_t<T> a) {
	return a.x * a.x + a.y * a.y + a.z * a.z + a.w * a.w;
}

template <typename T>
inline T norm(AzCore::vec4_t<T> a) {
	return sqrt(a.x * a.x + a.y * a.y + a.z * a.z + a.w * a.w);
}

template <bool isSegment, typename T>
T distSqrToLine(AzCore::vec4_t<T> segA, AzCore::vec4_t<T> segB, AzCore::vec4_t<T> point) {
	const AzCore::vec4_t<T> diff = segA - segB;
	const T lengthSquared = normSqr(diff);
	const T t = dot(diff, segA - point) / lengthSquared;
	AzCore::vec4_t<T> projection;
	if constexpr (isSegment) {
		if (t < T(0)) {
			projection = segA;
		} else if (t > T(1)) {
			projection = segB;
		} else {
			projection = segA - diff * t;
		}
	} else {
		projection = segA - diff * t;
	}
	return normSqr(point - projection);
}

template <typename T>
inline AzCore::vec4_t<T> normalize(AzCore::vec4_t<T> a, T epsilon=T(1.0e-12), AzCore::vec4_t<T> def={T(1), T(0), T(0), T(0)}) {
	T mag = norm(a);
	return mag < epsilon ? def : a / mag;
}

#endif // AZCORE_MATH_VEC4_HPP
