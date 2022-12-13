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
};

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

#endif // AZCORE_MATH_VEC2_HPP
