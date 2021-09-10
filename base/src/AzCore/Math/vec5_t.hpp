/*
	File: vec5_t.hpp
	Author: Philip Haynes
*/

#ifndef AZCORE_MATH_VEC5_HPP
#define AZCORE_MATH_VEC5_HPP

#include "basic.hpp"
#include "vec4_t.hpp"

namespace AzCore {

template <typename T>
struct vec5_t {
	union {
		struct {
			T x, y, z, w, v;
		};
		struct {
			T data[5];
		};
	};

	vec5_t() = default;
	inline vec5_t(T vec) : x(vec), y(vec), z(vec), w(vec), v(vec) {}
	inline vec5_t(T v1, T v2, T v3, T v4, T v5) : x(v1), y(v2), z(v3), w(v4), v(v5) {}
	template <typename I>
	vec5_t(vec5_t<I> a) : x((T)a.x), y((T)a.y), z((T)a.z), w((T)a.w), v((T)a.v) {}
	inline vec5_t<T> operator+(vec5_t<T> vec) const { return vec5_t<T>(x + vec.x, y + vec.y, z + vec.z, w + vec.w, v + vec.v); }
	inline vec5_t<T> operator-(vec5_t<T> vec) const { return vec5_t<T>(x - vec.x, y - vec.y, z - vec.z, w - vec.w, v - vec.v); }
	inline vec5_t<T> operator-() const { return vec5_t<T>(-x, -y, -z, -w, -v); }
	inline vec5_t<T> operator*(vec5_t<T> vec) const { return vec5_t<T>(x * vec.x, y * vec.y, z * vec.z, w * vec.w, v * vec.v); }
	inline vec5_t<T> operator/(vec5_t<T> vec) const { return vec5_t<T>(x / vec.x, y / vec.y, z / vec.z, w / vec.w, v / vec.v); }
	inline vec5_t<T> operator*(T vec) const { return vec5_t<T>(x * vec, y * vec, z * vec, w * vec, v * vec); }
	inline vec5_t<T> operator/(T vec) const { return vec5_t<T>(x / vec, y / vec, z / vec, w / vec, v / vec); }
	inline bool operator==(vec4_t<T> a) const { return x == a.x && y == a.y && z == a.z && w == a.w && v == a.v; }
	inline bool operator!=(vec4_t<T> a) const { return x != a.x || y != a.y || z != a.z || w != a.w || v != a.v; }
	inline T &operator[](u32 i) { return data[i]; }
	inline vec5_t<T> operator+=(vec5_t<T> vec) {
		x += vec.x;
		y += vec.y;
		z += vec.z;
		w += vec.w;
		v += vec.v;
		return *this;
	}
	inline vec5_t<T> operator-=(vec5_t<T> vec) {
		x -= vec.x;
		y -= vec.y;
		z -= vec.z;
		w -= vec.w;
		v -= vec.v;
		return *this;
	}
	inline vec5_t<T> operator/=(vec5_t<T> vec) {
		x /= vec.x;
		y /= vec.y;
		z /= vec.z;
		w /= vec.w;
		v /= vec.v;
		return *this;
	}
	inline vec5_t<T> operator/=(T vec) {
		x /= vec;
		y /= vec;
		z /= vec;
		w /= vec;
		v /= vec;
		return *this;
	}
	inline vec5_t<T> operator*=(vec5_t<T> vec) {
		x *= vec.x;
		y *= vec.y;
		z *= vec.z;
		w *= vec.w;
		v *= vec.v;
		return *this;
	}
	inline vec5_t<T> operator*=(T vec) {
		x *= vec;
		y *= vec;
		z *= vec;
		w *= vec;
		v *= vec;
		return *this;
	}
};

} // namespace AzCore

template <typename T>
inline AzCore::vec5_t<T> operator*(T a, AzCore::vec5_t<T> b) {
	return b * a;
}

template <typename T>
inline T dot(AzCore::vec5_t<T> a, AzCore::vec5_t<T> b) {
	return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w + a.v * b.v;
}

template <typename T>
inline T absSqr(AzCore::vec5_t<T> a) {
	return a.x * a.x + a.y * a.y + a.z * a.z + a.w * a.w + a.v * a.v;
}

template <typename T>
inline T abs(AzCore::vec5_t<T> a) {
	return sqrt(a.x * a.x + a.y * a.y + a.z * a.z + a.w * a.w + a.v * a.v);
}

template <bool isSegment, typename T>
T distSqrToLine(AzCore::vec5_t<T> segA, AzCore::vec5_t<T> segB, AzCore::vec5_t<T> point) {
	const AzCore::vec5_t<T> diff = segA - segB;
	const T lengthSquared = absSqr(diff);
	const T t = dot(diff, segA - point) / lengthSquared;
	AzCore::vec5_t<T> projection;
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
	return absSqr(point - projection);
}

#endif // AZCORE_MATH_VEC5_HPP
