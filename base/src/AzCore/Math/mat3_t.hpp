/*
	File: mat3_t.hpp
	Author: Philip Haynes
*/

#ifndef AZCORE_MATH_MAT3_T_HPP
#define AZCORE_MATH_MAT3_T_HPP

#include "mat2_t.hpp"
#include "vec3_t.hpp"

namespace AzCore {

// 3x3 matrix with the conventions matching GLSL
// - column-major memory layout
// - post-multiplication (transforms are applied in right-to-left order)
// - multiplication means lhs rows are dotted with rhs columns
// - vectors are row vectors on the lhs, and column vectors on the rhs
template <typename T>
struct mat3_t {
	union {
		vec3_t<T> cols[3];
		T data[9];
	};
	mat3_t() = default;
	inline mat3_t(mat2_t<T> in) : data{
		in.cols[0][0], in.cols[0][1], 0,
		in.cols[1][0], in.cols[1][1], 0,
		0            , 0            , 1
	} {}
	inline mat3_t(T a) : data{a, 0, 0, 0, a, 0, 0, 0, a} {}
	inline mat3_t(
		T col_0_x, T col_0_y, T col_0_z,
		T col_1_x, T col_1_y, T col_1_z,
		T col_2_x, T col_2_y, T col_2_z
	) : data{
		col_0_x, col_0_y, col_0_z,
		col_1_x, col_1_y, col_1_z,
		col_2_x, col_2_y, col_2_z
	} {}
	inline static mat3_t<T> FromCols(vec3_t<T> col_0, vec3_t<T> col_1, vec3_t<T> col_2) {
		mat3_t<T> result(
			col_0.x, col_0.y, col_0.z,
			col_1.x, col_1.y, col_1.z,
			col_2.x, col_2.y, col_2.z
		);
		return result;
	}
	inline static mat3_t<T> FromRows(vec3_t<T> row_0, vec3_t<T> row_1, vec3_t<T> row_2) {
		mat3_t<T> result(
			row_0.x, row_1.x, row_2.x,
			row_0.y, row_1.y, row_2.y,
			row_0.z, row_1.z, row_2.z
		);
		return result;
	}
	inline mat3_t(const T d[9]) : data{d[0], d[1], d[2], d[3], d[4], d[5], d[6], d[7], d[9]} {}
	inline vec3_t<T>& operator[](i32 column) {
		AzAssert(column >= 0 && column < 3, Stringify("Invalid column (", column, ") in mat3_t::operator[]"));
		return cols[column];
	}
	inline const vec3_t<T>& operator[](i32 column) const {
		AzAssert(column >= 0 && column < 3, Stringify("Invalid column (", column, ") in mat3_t::operator[]"));
		return cols[column];
	}
	template<i32 col>
	inline vec3_t<T> Col() const {
		static_assert(col >= 0 && col < 3);
		return cols[col];
	}
	template<i32 row>
	inline vec3_t<T> Row() const {
		static_assert(row >= 0 && row < 3);
		return vec3_t<T>(cols[0][row], cols[1][row], cols[2][row]);
	}
	inline static mat3_t<T> Identity() {
		return mat3_t(1);
	};
	// Only useful for rotations about aligned axes, such as {1, 0, 0}
	static mat3_t<T> RotationBasic(T angle, Axis axis) {
		T s = sin(angle), c = cos(angle);
		switch (axis) {
		case Axis::X:
			return mat3_t<T>(
				1, 0, 0,
				0, c, s,
				0,-s, c
			);
		case Axis::Y:
			return mat3_t<T>(
				c, 0,-s,
				0, 1, 0,
				s, 0, c
			);
		case Axis::Z:
			return mat3_t<T>(
				 c, s, 0,
				-s, c, 0,
				 0, 0, 1
			);
		}
		return mat3_t<T>();
	}
	// Useful for arbitrary axes
	static mat3_t<T> Rotation(T angle, vec3_t<T> axis) {
		T s = sin(angle), c = cos(angle);
		T ic = 1 - c;
		vec3_t<T> a = normalize(axis);
		T xx = square(a.x), yy = square(a.y), zz = square(a.z),
		  xy = a.x * a.y, xz = a.x * a.z, yz = a.y * a.z;
		return mat3_t<T>(
			c + xx * ic,         xy * ic + a.z * s,   xz * ic - a.y * s,
			xy * ic - a.z * s,   c + yy * ic,         yz * ic + a.x * s,
			xz * ic + a.y * s,   yz * ic - a.x * s,   c + zz * ic
		);
	}
	static mat3_t<T> Scale(vec3_t<T> scale) {
		return mat3_t<T>(
			scale.x, 0, 0,
			0, scale.y, 0,
			0, 0, scale.z
		);
	}
	inline mat3_t<T> Transpose() const {
		return FromRows(
			Col<0>(),
			Col<1>(),
			Col<2>()
		);
	}
	inline mat3_t<T> operator+(mat3_t<T> rhs) const {
		return FromCols(
			Col<0>() + rhs.template Col<0>(),
			Col<1>() + rhs.template Col<1>(),
			Col<2>() + rhs.template Col<2>()
		);
	}
	inline mat3_t<T> operator*(mat3_t<T> rhs) const {
		return mat3_t<T>(
			dot(Row<0>(), rhs.template Col<0>()), dot(Row<1>(), rhs.template Col<0>()), dot(Row<2>(), rhs.template Col<0>()),
			dot(Row<0>(), rhs.template Col<1>()), dot(Row<1>(), rhs.template Col<1>()), dot(Row<2>(), rhs.template Col<1>()),
			dot(Row<0>(), rhs.template Col<2>()), dot(Row<1>(), rhs.template Col<2>()), dot(Row<2>(), rhs.template Col<2>())
		);
	}
	inline vec3_t<T> operator*(vec3_t<T> rhs) const {
		return vec3_t<T>(
			dot(Row<0>(), rhs),
			dot(Row<1>(), rhs),
			dot(Row<2>(), rhs)
		);
	}
	inline mat3_t<T> operator*(T a) const {
		return FromCols(
			Col<0>() * a,
			Col<1>() * a,
			Col<2>() * a
		);
	}
	inline mat3_t<T> operator/(T a) const {
		return FromCols(
			Col<0>() / a,
			Col<1>() / a,
			Col<2>() / a
		);
	}
};

typedef mat3_t<f32> mat3;
typedef mat3_t<f64> mat3d;

} // namespace AzCore

template <typename T>
inline AzCore::vec3_t<T> operator*(AzCore::vec3_t<T> lhs, AzCore::mat3_t<T> rhs) {
	return AzCore::vec3_t<T>(
		dot(lhs, rhs.template Col<0>()),
		dot(lhs, rhs.template Col<1>()),
		dot(lhs, rhs.template Col<2>())
	);
}

#endif // AZCORE_MATH_MAT3_T_HPP
