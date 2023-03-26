/*
	File: vec2_t.hpp
	Author: Philip Haynes
*/

#ifndef AZCORE_MATH_VEC2_HPP
#define AZCORE_MATH_VEC2_HPP

#include "basic.hpp"

namespace AzCore {

template <typename T>
struct vec2_t {
	union {
		struct {
			T x, y;
		};
		struct {
			T u, v;
		};
		struct {
			T data[2];
		};
	};

	vec2_t() = default;
	inline vec2_t(T a) : x(a), y(a){};
	inline vec2_t(T a, T b) : x(a), y(b){};
	template <typename I>
	vec2_t(vec2_t<I> a) : x((T)a.x), y((T)a.y) {}
	inline vec2_t<T> operator+(vec2_t<T> a) const {
		return vec2_t<T>(x + a.x, y + a.y);
	}
	inline vec2_t<T> operator-(vec2_t<T> a) const {
		return vec2_t<T>(x - a.x, y - a.y);
	}
	inline vec2_t<T> operator-() const { return vec2_t<T>(-x, -y); }
	inline vec2_t<T> operator*(vec2_t<T> a) const {
		return vec2_t<T>(x * a.x, y * a.y);
	}
	inline vec2_t<T> operator/(vec2_t<T> a) const {
		return vec2_t<T>(x / a.x, y / a.y);
	}
	inline vec2_t<T> operator*(T a) const {
		return vec2_t<T>(x * a, y * a);
	}
	inline vec2_t<T> operator/(T a) const {
		return vec2_t<T>(x / a, y / a);
	}
	inline bool operator==(vec2_t<T> a) const {
		return x == a.x && y == a.y;
	}
	inline bool operator!=(vec2_t<T> a) const {
		return x != a.x || y != a.y;
	}
	inline T &operator[](u32 i) {
		return data[i];
	}
	inline vec2_t<T> operator+=(vec2_t<T> a) {
		x += a.x;
		y += a.y;
		return *this;
	}
	inline vec2_t<T> operator-=(vec2_t<T> a) {
		x -= a.x;
		y -= a.y;
		return *this;
	}
	inline vec2_t<T> operator/=(vec2_t<T> a) {
		x /= a.x;
		y /= a.y;
		return *this;
	}
	inline vec2_t<T> operator/=(T a) {
		x /= a;
		y /= a;
		return *this;
	}
	inline vec2_t<T> operator*=(vec2_t<T> a) {
		x *= a.x;
		y *= a.y;
		return *this;
	}
	inline vec2_t<T> operator*=(T a) {
		x *= a;
		y *= a;
		return *this;
	}
	// Angle points in the +x direction at 0 and +y direction at tau/4
	static inline vec2_t<T> UnitVecFromAngle(T angle) {
		vec2_t<T> result;
		result.x = cos(angle);
		result.y = sin(angle);
		return result;
	}
};

typedef vec2_t<f32> vec2;
typedef vec2_t<f64> vec2d;
typedef vec2_t<i32> vec2i;

} // namespace AzCore

template <typename T>
inline AzCore::vec2_t<T> operator*(T a, AzCore::vec2_t<T> b) {
	return b * a;
}

template <typename T>
inline T dot(AzCore::vec2_t<T> a, AzCore::vec2_t<T> b) {
	return a.x * b.x + a.y * b.y;
}

template <typename T>
inline T normSqr(AzCore::vec2_t<T> a) {
	return a.x * a.x + a.y * a.y;
}

template <typename T>
inline T norm(AzCore::vec2_t<T> a) {
	return sqrt(a.x * a.x + a.y * a.y);
}

template <bool isSegment, typename T>
inline T distSqrToLine(AzCore::vec2_t<T> segA, AzCore::vec2_t<T> segB, AzCore::vec2_t<T> point) {
	const AzCore::vec2_t<T> diff = segA - segB;
	const T lengthSquared = normSqr(diff);
	const T t = dot(diff, segA - point) / lengthSquared;
	AzCore::vec2_t<T> projection;
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
inline AzCore::vec2_t<T> normalize(AzCore::vec2_t<T> a, T epsilon=T(1.0e-12), AzCore::vec2_t<T> def={T(1), T(0)}) {
	T mag = norm(a);
	return mag < epsilon ? def : a / mag;
}

template <typename T>
constexpr void barycentricCoords(az::vec2_t<T> a, az::vec2_t<T> b, az::vec2_t<T> c, az::vec2_t<T> p, T &dstU, T &dstV, T &dstW) {
	T denom = (b.y-c.y)*(a.x-c.x) + (c.x-b.x)*(a.y-c.y);
	dstU = ((b.y-c.y)*(p.x-c.x) + (c.x-b.x)*(p.y-c.y)) / denom;
	dstV = ((c.y-a.y)*(p.x-c.x) + (a.x-c.x)*(p.y-c.y)) / denom;
	dstW = T(1) - dstU - dstV;
}

// Interpolates a triangle defined by a, b, c, with barycentric coordinates u, v, w
// Note that this only works if u+v+w = 1
template <typename T, typename F>
constexpr F barycentricInterp(az::vec2_t<F> a, az::vec2_t<F> b, az::vec2_t<F> c, az::vec2_t<F> p, T a_val, T b_val, T c_val) {
	F u, v, w;
	barycentricCoords(a, b, c, p, u, v, w);
	// AzAssert(abs(u + v + w - F(1)) < F(0.0001), "Barycentric coordinates invalid");
	return a_val * u + b_val * v + c_val * w;
}

#endif // AZCORE_MATH_VEC2_HPP
