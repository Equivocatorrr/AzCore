/*
	File: mat2_t.hpp
	Author: Philip Haynes
*/

#ifndef AZCORE_MATH_MAT2_T_HPP
#define AZCORE_MATH_MAT2_T_HPP

#include "vec2_t.hpp"

namespace AzCore {

// 2x2 matrix with the data in a column-major layout to match GLSL's behavior
template <typename T>
struct mat2_t {
	union {
		struct {
			T x1, y1,
			  x2, y2;
		} h;
		struct {
			T x1, x2,
			  y1, y2;
		} v;
		struct {
			T data[4];
		};
	};
	mat2_t() = default;
	inline mat2_t(T a) : data{
		a, 0,
		0, a
	} {};
	inline mat2_t(
		T x1, T y1,
		T x2, T y2
	) : data{
		x1, y1,
		x2, y2
	} {};
	inline static mat2_t<T> FromCols(vec2_t<T> col1, vec2_t<T> col2) {
		mat2_t<T> result(
			col1.x, col1.y,
			col2.x, col2.y
		);
		return result;
	}
	inline static mat2_t<T> FromRows(vec2_t<T> row1, vec2_t<T> row2) {
		mat2_t<T> result(
			row1.x, row1.y,
			row2.x, row2.y
		);
		return result;
	}
	inline mat2_t(const T d[4]) : data{d[0], d[1], d[2], d[3]} {};
	inline vec2_t<T> Col1() const { return vec2_t<T>(h.x1, h.y1); }
	inline vec2_t<T> Col2() const { return vec2_t<T>(h.x2, h.y2); }
	inline vec2_t<T> Row1() const { return vec2_t<T>(v.x1, v.y1); }
	inline vec2_t<T> Row2() const { return vec2_t<T>(v.x2, v.y2); }
	inline static mat2_t<T> Identity() {
		return mat2_t(1);
	};
	inline static mat2_t<T> Rotation(T angle) {
		T s = sin(angle), c = cos(angle);
		return mat2_t<T>(
			 c, s,
			-s, c
		);
	}
	inline static mat2_t<T> Skewer(vec2_t<T> amount) {
		return mat2_t<T>(
			1, amount.y,
			amount.x, 1
		);
	}
	inline static mat2_t<T> Scaler(vec2_t<T> scale) {
		return mat2_t<T>(
			scale.x, 0,
			0, scale.y
		);
	}
	inline mat2_t<T> Transpose() const {
		return mat2_t<T>(
			v.x1, v.y1,
			v.x2, v.y2
		);
	}
	inline mat2_t<T> operator+(mat2_t<T> rhs) const {
		return mat2_t<T>(
			h.x1 + rhs.h.x1, h.y1 + rhs.h.y1,
			h.x2 + rhs.h.x2, h.y2 + rhs.h.y2
		);
	}
	inline mat2_t<T> operator*(mat2_t<T> rhs) const {
		return mat2_t<T>(
			dot(Row1(), rhs.Col1()), dot(Row2(), rhs.Col1()),
			dot(Row1(), rhs.Col2()), dot(Row2(), rhs.Col2())
		);
	}
	inline vec2_t<T> operator*(vec2_t<T> rhs) const {
		return vec2_t<T>(
			dot(Row1(), rhs),
			dot(Row2(), rhs)
		);
	}
	inline mat2_t<T> operator*(T rhs) const {
		return mat2_t<T>(
			h.x1 * rhs, h.y1 * rhs,
			h.x2 * rhs, h.y2 * rhs
		);
	}
	inline mat2_t<T> operator/(T rhs) const {
		return mat2_t<T>(
			h.x1 / rhs, h.y1 / rhs,
			h.x2 / rhs, h.y2 / rhs
		);
	}
};

typedef mat2_t<f32> mat2;
typedef mat2_t<f64> mat2d;

} // namespace AzCore

template <typename T>
inline AzCore::vec2_t<T> operator*(AzCore::vec2_t<T> lhs, AzCore::mat2_t<T> rhs) {
	return AzCore::vec2_t<T>(
		dot(lhs, rhs.Col1()),
		dot(lhs, rhs.Col2())
	);
}

#endif // AZCORE_MATH_MAT2_T_HPP
