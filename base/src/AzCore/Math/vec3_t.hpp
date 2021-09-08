/*
	File: vec3_t.hpp
	Author: Philip Haynes
*/

#ifndef AZCORE_MATH_VEC3_HPP
#define AZCORE_MATH_VEC3_HPP

#include "basic.hpp"

namespace AzCore {

template <typename T>
struct vec3_t {
	union {
		struct {
			T x, y, z;
		};
		struct {
			T r, g, b;
		};
		struct {
			T h, s, v;
		};
		struct {
			T data[3];
		};
	};

	vec3_t() = default;
	inline vec3_t(const T &a) : x(a), y(a), z(a) {}
	inline vec3_t(const T &v1, const T &v2, const T &v3) : x(v1), y(v2), z(v3) {}
	template <typename I>
	inline vec3_t(const vec3_t<I> &a) : x((T)a.x), y((T)a.y), z((T)a.z) {}
	inline vec3_t<T> operator+(const vec3_t<T> &a) const { return vec3_t<T>(x + a.x, y + a.y, z + a.z); }
	inline vec3_t<T> operator-(const vec3_t<T> &a) const { return vec3_t<T>(x - a.x, y - a.y, z - a.z); }
	inline vec3_t<T> operator-() const { return vec3_t<T>(-x, -y, -z); }
	inline vec3_t<T> operator*(const vec3_t<T> &a) const { return vec3_t<T>(x * a.x, y * a.y, z * a.z); }
	inline vec3_t<T> operator/(const vec3_t<T> &a) const { return vec3_t<T>(x / a.x, y / a.y, z / a.z); }
	inline vec3_t<T> operator*(const T &a) const { return vec3_t<T>(x * a, y * a, z * a); }
	inline vec3_t<T> operator/(const T &a) const { return vec3_t<T>(x / a, y / a, z / a); }
	inline bool operator==(const vec3_t<T> &a) const { return x == a.x && y == a.y && z == a.z; }
	inline bool operator!=(const vec3_t<T> &a) const { return x != a.x || y != a.y || z != a.z; }
	inline T &operator[](const u32 &i) { return data[i]; }
	inline vec3_t<T> operator+=(const vec3_t<T> &a) {
		x += a.x;
		y += a.y;
		z += a.z;
		return *this;
	}
	inline vec3_t<T> operator-=(const vec3_t<T> &a) {
		x -= a.x;
		y -= a.y;
		z -= a.z;
		return *this;
	}
	inline vec3_t<T> operator/=(const vec3_t<T> &a) {
		x /= a.x;
		y /= a.y;
		z /= a.z;
		return *this;
	}
	inline vec3_t<T> operator/=(const T &a) {
		x /= a;
		y /= a;
		z /= a;
		return *this;
	}
	inline vec3_t<T> operator*=(const vec3_t<T> &a) {
		x *= a.x;
		y *= a.y;
		z *= a.z;
		return *this;
	}
	inline vec3_t<T> operator*=(const T &a) {
		x *= a;
		y *= a;
		z *= a;
		return *this;
	}
};


template <typename T>
vec3_t<T> hsvToRgb(vec3_t<T> hsv);

template <typename T>
vec3_t<T> rgbToHsv(vec3_t<T> rgb);

} // namespace AzCore

template <typename T>
inline AzCore::vec3_t<T> operator*(const T &a, const AzCore::vec3_t<T> &b) {
	return b * a;
}

template <typename T>
inline AzCore::vec3_t<T> cross(const AzCore::vec3_t<T> &a, const AzCore::vec3_t<T> &b) {
	return AzCore::vec3_t<T>(
		a.y * b.z - a.z * b.y,
		a.z * b.x - a.x * b.z,
		a.x * b.y - a.y * b.x);
}

template <typename T>
inline T dot(const AzCore::vec3_t<T> &a, const AzCore::vec3_t<T> &b) {
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

template <typename T>
inline T absSqr(const AzCore::vec3_t<T> &a) {
	return a.x * a.x + a.y * a.y + a.z * a.z;
}

template <typename T>
inline T abs(const AzCore::vec3_t<T> &a) {
	return sqrt(a.x * a.x + a.y * a.y + a.z * a.z);
}

template <bool isSegment, typename T>
T distSqrToLine(const AzCore::vec3_t<T> &segA, const AzCore::vec3_t<T> &segB, const AzCore::vec3_t<T> &point) {
	const AzCore::vec3_t<T> diff = segA - segB;
	const T lengthSquared = absSqr(diff);
	const T t = dot(diff, segA - point) / lengthSquared;
	AzCore::vec3_t<T> projection;
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

#endif // AZCORE_MATH_VEC3_HPP
