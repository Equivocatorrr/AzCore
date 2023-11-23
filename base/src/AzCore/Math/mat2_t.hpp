/*
	File: mat2_t.hpp
	Author: Philip Haynes
*/

#ifndef AZCORE_MATH_MAT2_T_HPP
#define AZCORE_MATH_MAT2_T_HPP

#include "vec2_t.hpp"

namespace AzCore {

// 2x2 matrix with the conventions matching GLSL
// - column-major memory layout
// - post-multiplication (transforms are applied in right-to-left order)
// - multiplication means lhs rows are dotted with rhs columns
// - vectors are row vectors on the lhs, and column vectors on the rhs
template <typename T>
struct mat2_t {
	union {
		vec2_t<T> cols[2];
		T data[4];
	};
	mat2_t() = default;
	inline mat2_t(T a) : data{
		a, 0,
		0, a
	} {};
	inline mat2_t(
		T col_0_x, T col_0_y,
		T col_1_x, T col_1_y
	) : data{
		col_0_x, col_0_y,
		col_1_x, col_1_y
	} {};
	inline static mat2_t<T> FromCols(vec2_t<T> col_0, vec2_t<T> col_1) {
		mat2_t<T> result(
			col_0.x, col_0.y,
			col_1.x, col_1.y
		);
		return result;
	}
	inline static mat2_t<T> FromRows(vec2_t<T> row_0, vec2_t<T> row_1) {
		mat2_t<T> result(
			row_0.x, row_1.x,
			row_0.y, row_1.y
		);
		return result;
	}
	inline mat2_t(const T d[4]) : data{d[0], d[1], d[2], d[3]} {};
	inline vec2_t<T>& operator[](i32 column) {
		AzAssert(column >= 0 && column < 2, Stringify("Invalid column (", column, ") in mat2_t::operator[]"));
		return cols[column];
	}
	inline const vec2_t<T>& operator[](i32 column) const {
		AzAssert(column >= 0 && column < 2, Stringify("Invalid column (", column, ") in mat2_t::operator[]"));
		return cols[column];
	}
	template<i32 col>
	inline vec2_t<T> Col() const {
		static_assert(col >= 0 && col < 2);
		return cols[col];
	}
	template<i32 row>
	inline vec2_t<T> Row() const {
		static_assert(row >= 0 && row < 2);
		return vec2_t<T>(cols[0][row], cols[1][row]);
	}
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
	inline static mat2_t<T> Scale(vec2_t<T> scale) {
		return mat2_t<T>(
			scale.x, 0,
			0, scale.y
		);
	}
	inline mat2_t<T> Transpose() const {
		return FromRows(
			Col<0>(),
			Col<1>()
		);
	}
	inline mat2_t<T> operator+(mat2_t<T> rhs) const {
		return FromCols(
			Col<0>() + rhs.Col<0>(),
			Col<1>() + rhs.Col<1>()
		);
	}
	inline mat2_t<T> operator*(mat2_t<T> rhs) const {
		return mat2_t<T>(
			dot(Row<0>(), rhs.Col<0>()), dot(Row<1>(), rhs.Col<0>()),
			dot(Row<0>(), rhs.Col<1>()), dot(Row<1>(), rhs.Col<1>())
		);
	}
	inline vec2_t<T> operator*(vec2_t<T> rhs) const {
		return vec2_t<T>(
			dot(Row<0>(), rhs),
			dot(Row<1>(), rhs)
		);
	}
	inline mat2_t<T> operator*(T a) const {
		return FromCols(
			Col<0>() * a,
			Col<1>() * a
		);
	}
	inline mat2_t<T> operator/(T a) const {
		return FromCols(
			Col<0>() / a,
			Col<1>() / a
		);
	}
};

typedef mat2_t<f32> mat2;
typedef mat2_t<f64> mat2d;

} // namespace AzCore

template <typename T>
inline AzCore::vec2_t<T> operator*(AzCore::vec2_t<T> lhs, AzCore::mat2_t<T> rhs) {
	return AzCore::vec2_t<T>(
		dot(lhs, rhs.Col<0>()),
		dot(lhs, rhs.Col<1>())
	);
}

#endif // AZCORE_MATH_MAT2_T_HPP
