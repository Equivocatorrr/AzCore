/*
	File: vec3_t.hpp
	Author: Philip Haynes
*/

#ifndef AZCORE_MATH_VEC3_HPP
#define AZCORE_MATH_VEC3_HPP

#include "vec2_t.hpp"

#include "basic.hpp"

namespace AzCore {

template <typename T>
struct vec3_t {
	union {
		struct {
			T x, y, z;
		};
		struct {
			vec2_t<T> xy;
		};
		struct {
			T __x; vec2_t<T> yz;
		};
		struct {
			T r, g, b;
		};
		struct {
			vec2_t<T> rg;
		};
		struct {
			T __r; vec2_t<T> gb;
		};
		struct {
			T h, s, v;
		};
		struct {
			T data[3];
		};
	};

	vec3_t() = default;
	inline vec3_t(T a) : x(a), y(a), z(a) {}
	inline vec3_t(vec2_t<T> _xy, T _z) : x(_xy.x), y(_xy.y), z(_z) {}
	inline vec3_t(T _x, vec2_t<T> _yz) : x(_x), y(_yz.x), z(_yz.y) {}
	inline vec3_t(T v1, T v2, T v3) : x(v1), y(v2), z(v3) {}
	template <typename I>
	inline vec3_t(vec3_t<I> a) : x((T)a.x), y((T)a.y), z((T)a.z) {}
	inline vec3_t<T> operator+(vec3_t<T> a) const { return vec3_t<T>(x + a.x, y + a.y, z + a.z); }
	inline vec3_t<T> operator-(vec3_t<T> a) const { return vec3_t<T>(x - a.x, y - a.y, z - a.z); }
	inline vec3_t<T> operator-() const { return vec3_t<T>(-x, -y, -z); }
	inline vec3_t<T> operator*(vec3_t<T> a) const { return vec3_t<T>(x * a.x, y * a.y, z * a.z); }
	inline vec3_t<T> operator/(vec3_t<T> a) const { return vec3_t<T>(x / a.x, y / a.y, z / a.z); }
	inline vec3_t<T> operator*(T a) const { return vec3_t<T>(x * a, y * a, z * a); }
	inline vec3_t<T> operator/(T a) const { return vec3_t<T>(x / a, y / a, z / a); }
	inline bool operator==(vec3_t<T> a) const { return x == a.x && y == a.y && z == a.z; }
	inline bool operator!=(vec3_t<T> a) const { return x != a.x || y != a.y || z != a.z; }
	inline T& operator[](i32 i) { return data[i]; }
	inline const T& operator[](i32 i) const { return data[i]; }
	inline vec3_t<T> operator+=(vec3_t<T> a) {
		x += a.x;
		y += a.y;
		z += a.z;
		return *this;
	}
	inline vec3_t<T> operator-=(vec3_t<T> a) {
		x -= a.x;
		y -= a.y;
		z -= a.z;
		return *this;
	}
	inline vec3_t<T> operator/=(vec3_t<T> a) {
		x /= a.x;
		y /= a.y;
		z /= a.z;
		return *this;
	}
	inline vec3_t<T> operator/=(T a) {
		x /= a;
		y /= a;
		z /= a;
		return *this;
	}
	inline vec3_t<T> operator*=(vec3_t<T> a) {
		x *= a.x;
		y *= a.y;
		z *= a.z;
		return *this;
	}
	inline vec3_t<T> operator*=(T a) {
		x *= a;
		y *= a;
		z *= a;
		return *this;
	}
	inline vec3_t<T> RotatedXPos90() const {
		return vec3_t<T>(x, -z, y);
	}
	inline vec3_t<T> RotatedXNeg90() const {
		return vec3_t<T>(x, z, -y);
	}
	inline vec3_t<T> RotatedYPos90() const {
		return vec3_t<T>(z, y, -x);
	}
	inline vec3_t<T> RotatedYNeg90() const {
		return vec3_t<T>(-z, y, x);
	}
	inline vec3_t<T> RotatedZPos90() const {
		return vec3_t<T>(-y, x, z);
	}
	inline vec3_t<T> RotatedZNeg90() const {
		return vec3_t<T>(y, -x, z);
	}
};

typedef vec3_t<f32> vec3;
typedef vec3_t<f64> vec3d;
typedef vec3_t<i32> vec3i;

} // namespace AzCore

template <typename T>
inline AzCore::vec3_t<T> operator*(T a, AzCore::vec3_t<T> b) {
	return b * a;
}

template <typename T>
inline AzCore::vec3_t<T> cross(AzCore::vec3_t<T> a, AzCore::vec3_t<T> b) {
	return AzCore::vec3_t<T>(
		a.y * b.z - a.z * b.y,
		a.z * b.x - a.x * b.z,
		a.x * b.y - a.y * b.x);
}

template <typename T>
inline T dot(AzCore::vec3_t<T> a, AzCore::vec3_t<T> b) {
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

template <typename T>
inline T normSqr(AzCore::vec3_t<T> a) {
	return a.x * a.x + a.y * a.y + a.z * a.z;
}

template <typename T>
inline T norm(AzCore::vec3_t<T> a) {
	return sqrt(a.x * a.x + a.y * a.y + a.z * a.z);
}

template <bool isSegment, typename T>
T distSqrToLine(AzCore::vec3_t<T> segA, AzCore::vec3_t<T> segB, AzCore::vec3_t<T> point) {
	const AzCore::vec3_t<T> diff = segA - segB;
	const T lengthSquared = normSqr(diff);
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
	return normSqr(point - projection);
}

template <typename T>
inline AzCore::vec3_t<T> normalize(AzCore::vec3_t<T> a, T epsilon=T(1.0e-12), AzCore::vec3_t<T> def={T(1), T(0), T(0)}) {
	T mag = norm(a);
	return mag < epsilon ? def : a / mag;
}

// Returns a adjusted to be orthogonal to ref and normalized
template <typename T>
inline AzCore::vec3_t<T> orthogonalize(AzCore::vec3_t<T> a, AzCore::vec3_t<T> ref, T epsilon=T(1.0e-7)) {
	a = normalize(a);
	ref = normalize(ref);
	T dp = dot(a, ref);
	T p = T(1);
	// For a = ref = (1, 2, 3) or any scaled version, this will need to loop twice. For any other direction this will loop a maximum of once, and probably not at all.
	while (abs(dp) >= T(1) - epsilon) {
		a.x = p;
		p += T(1);
		a.y = p;
		p += T(1);
		a.z = p;
		p += T(1);
		a = normalize(a);
		dp = dot(a, ref);
	}
	return normalize(a - ref * dp);
}

inline AzCore::vec3 min(AzCore::vec3 a, AzCore::vec3 b) {
	alignas(16) f32 data[8] = {a[0], a[1], a[2], 0.0f, b[0], b[1], b[2], 0.0f};
	_mm_store_ps(data, _mm_min_ps(_mm_load_ps(data), _mm_load_ps(data+4)));
	return AzCore::vec3(data[0], data[1], data[2]);
}

inline AzCore::vec3 max(AzCore::vec3 a, AzCore::vec3 b) {
	alignas(16) f32 data[8] = {a[0], a[1], a[2], 0.0f, b[0], b[1], b[2], 0.0f};
	_mm_store_ps(data, _mm_max_ps(_mm_load_ps(data), _mm_load_ps(data+4)));
	return AzCore::vec3(data[0], data[1], data[2]);
}

inline AzCore::vec3 abs(AzCore::vec3 a) {
	alignas(16) f32 data[8] = {a[0], a[1], a[2], 0.0f, -a[0], -a[1], -a[2], 0.0f};
	_mm_store_ps(data, _mm_max_ps(_mm_load_ps(data), _mm_load_ps(data+4)));
	return AzCore::vec3(data[0], data[1], data[2]);
}

inline AzCore::vec3d min(AzCore::vec3d a, AzCore::vec3d b) {
	_mm_store_pd(a.data, _mm_min_pd(_mm_set_pd(a[0], a[1]), _mm_set_pd(b[0], b[1])));
	a.z = min(a.z, b.z);
	return a;
}

inline AzCore::vec3d max(AzCore::vec3d a, AzCore::vec3d b) {
	_mm_store_pd(a.data, _mm_max_pd(_mm_set_pd(a[0], a[1]), _mm_set_pd(b[0], b[1])));
	a.z = max(a.z, b.z);
	return a;
}

#endif // AZCORE_MATH_VEC3_HPP
