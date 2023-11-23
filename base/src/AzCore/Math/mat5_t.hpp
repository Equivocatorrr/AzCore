/*
	File: mat5_t.hpp
	Author: Philip Haynes
*/

#ifndef AZCORE_MATH_MAT5_T_HPP
#define AZCORE_MATH_MAT5_T_HPP

#include "vec3_t.hpp"
#include "vec5_t.hpp"
#include "mat4_t.hpp"

namespace AzCore {

// 5x5 matrix with the conventions matching GLSL
// - column-major memory layout
// - post-multiplication (transforms are applied in right-to-left order)
// - multiplication means lhs rows are dotted with rhs columns
// - vectors are row vectors on the lhs, and column vectors on the rhs
template <typename T>
struct mat5_t {
	union {
		vec5_t<T> cols[5];
		T data[25];
	};
	mat5_t() = default;
	inline mat5_t(mat4_t<T> in) : data{
		in.cols[0][0], in.cols[0][1], in.cols[0][2], in.cols[0][3], 0,
		in.cols[1][0], in.cols[1][1], in.cols[1][2], in.cols[1][3], 0,
		in.cols[2][0], in.cols[2][1], in.cols[2][2], in.cols[2][3], 0,
		in.cols[3][0], in.cols[3][1], in.cols[3][2], in.cols[3][3], 0,
		0,       0,       0,       0,       1
	} {}
	inline mat5_t(T a) : data{
		a, 0, 0, 0, 0,
		0, a, 0, 0, 0,
		0, 0, a, 0, 0,
		0, 0, 0, a, 0,
		0, 0, 0, 0, a
	} {}
	inline mat5_t(
		T col_0_x, T col_0_y, T col_0_z, T col_0_w, T col_0_v,
		T col_1_x, T col_1_y, T col_1_z, T col_1_w, T col_1_v,
		T col_2_x, T col_2_y, T col_2_z, T col_2_w, T col_2_v,
		T col_3_x, T col_3_y, T col_3_z, T col_3_w, T col_3_v,
		T col_4_x, T col_4_y, T col_4_z, T col_4_w, T col_4_v
	) : data{
		col_0_x, col_0_y, col_0_z, col_0_w, col_0_v,
		col_1_x, col_1_y, col_1_z, col_1_w, col_1_v,
		col_2_x, col_2_y, col_2_z, col_2_w, col_2_v,
		col_3_x, col_3_y, col_3_z, col_3_w, col_3_v,
		col_4_x, col_4_y, col_4_z, col_4_w, col_4_v
	} {}
	inline static mat5_t<T> FromCols(vec5_t<T> col_0, vec5_t<T> col_1, vec5_t<T> col_2, vec5_t<T> col_3, vec5_t<T> col_4) {
		mat5_t<T> result(
			col_0.x, col_0.y, col_0.z, col_0.w, col_0.v,
			col_1.x, col_1.y, col_1.z, col_1.w, col_1.v,
			col_2.x, col_2.y, col_2.z, col_2.w, col_2.v,
			col_3.x, col_3.y, col_3.z, col_3.w, col_3.v,
			col_4.x, col_4.y, col_4.z, col_4.w, col_4.v
		);
		return result;
	}
	inline static mat5_t<T> FromRows(vec5_t<T> row_0, vec5_t<T> row_1, vec5_t<T> row_2, vec5_t<T> row_3, vec5_t<T> row_4) {
		mat5_t<T> result(
			row_0.x, row_1.x, row_2.x, row_3.x, row_4.x,
			row_0.y, row_1.y, row_2.y, row_3.y, row_4.y,
			row_0.z, row_1.z, row_2.z, row_3.z, row_4.z,
			row_0.w, row_1.w, row_2.w, row_3.w, row_4.w,
			row_0.v, row_1.v, row_2.v, row_3.v, row_4.v
		);
		return result;
	}
	inline mat5_t(const T d[25]) : data{
		d[ 0], d[ 1], d[ 2], d[ 3], d[ 4],
		d[ 5], d[ 6], d[ 7], d[ 8], d[ 9],
		d[10], d[11], d[12], d[13], d[14],
		d[15], d[16], d[17], d[18], d[19],
		d[20], d[21], d[22], d[23], d[24]
	} {}
	inline vec5_t<T>& operator[](i32 column) {
		AzAssert(column >= 0 && column < 5, Stringify("Invalid column (", column, ") in mat5_t::operator[]"));
		return cols[column];
	}
	inline const vec5_t<T>& operator[](i32 column) const {
		AzAssert(column >= 0 && column < 5, Stringify("Invalid column (", column, ") in mat5_t::operator[]"));
		return cols[column];
	}
	template<i32 col>
	inline vec5_t<T> Col() const {
		static_assert(col >= 0 && col < 5);
		return cols[col];
	}
	template<i32 row>
	inline vec5_t<T> Row() const {
		static_assert(row >= 0 && row < 5);
		return vec5_t<T>(cols[0][row], cols[1][row], cols[2][row], cols[3][row], cols[4][row]);
	}
	inline static mat5_t<T> Identity() {
		return mat5_t(1);
	};
	// Only useful for rotations about aligned planes, such as {{1, 0, 0, 0}, {0, 0, 0, 1}}
	// Note: The planes stay fixed in place and everything else rotates around them.
	// Note: The V-Dimension is always fixed in place
	static mat5_t<T> RotationBasic(T angle, Plane plane) {
		T s = sin(angle), c = cos(angle);
		switch (plane) {
		case Plane::XW:
			return mat5_t<T>(
				1, 0, 0, 0, 0,
				0, c, s, 0, 0,
				0,-s, c, 0, 0,
				0, 0, 0, 1, 0,
				0, 0, 0, 0, 1
			);
		case Plane::YW:
			return mat5_t<T>(
				c, 0,-s, 0, 0,
				0, 1, 0, 0, 0,
				s, 0, c, 0, 0,
				0, 0, 0, 1, 0,
				0, 0, 0, 0, 1
			);
		case Plane::ZW:
			return mat5_t<T>(
				 c, s, 0, 0, 0,
				-s, c, 0, 0, 0,
				 0, 0, 1, 0, 0,
				 0, 0, 0, 1, 0,
				 0, 0, 0, 0, 1
			);
		case Plane::XY:
			return mat5_t<T>(
				1, 0, 0, 0, 0,
				0, 1, 0, 0, 0,
				0, 0, c, s, 0,
				0, 0,-s, c, 0,
				0, 0, 0, 0, 1
			);
		case Plane::YZ:
			return mat5_t<T>(
				 c, 0, 0, s, 0,
				 0, 1, 0, 0, 0,
				 0, 0, 1, 0, 0,
				-s, 0, 0, c, 0,
				 0, 0, 0, 0, 1
			);
		case Plane::ZX:
			return mat5_t<T>(
				1, 0, 0, 0, 0,
				0, c, 0,-s, 0,
				0, 0, 1, 0, 0,
				0, s, 0, c, 0,
				0, 0, 0, 0, 1
			);
		}
		return mat5_t<T>();
	}
	// For using 3D-axis rotations
	static mat5_t<T> RotationBasic(T angle, Axis axis) {
		switch (axis) {
		case Axis::X:
			return RotationBasic(angle, Plane::XW);
		case Axis::Y:
			return RotationBasic(angle, Plane::YW);
		case Axis::Z:
			return RotationBasic(angle, Plane::ZW);
		}
		return mat5_t<T>();
	}
	// Useful for arbitrary 3D-axes
	static mat5_t<T> Rotation(T angle, vec3_t<T> axis) {
		T s = sin(angle), c = cos(angle);
		T ic = 1 - c;
		vec3_t<T> a = normalize(axis);
		T xx = square(a.x), yy = square(a.y), zz = square(a.z),
		  xy = a.x * a.y, xz = a.x * a.z, yz = a.y * a.z;
		return mat5_t<T>(
			c + xx * ic,         xy * ic + a.z * s,   xz * ic - a.y * s,   0, 0,
			xy * ic - a.z * s,   c + yy * ic,         yz * ic + a.x * s,   0, 0,
			xz * ic + a.y * s,   yz * ic - a.x * s,   c + zz * ic,         0, 0,
			0,                   0,                   0,                   1, 0,
			0,                   0,                   0,                   0, 1
		);
	}
	static mat5_t<T> Scale(vec5_t<T> scale) {
		return mat5_t<T>(
			scale.x, 0, 0, 0, 0,
			0, scale.y, 0, 0, 0,
			0, 0, scale.z, 0, 0,
			0, 0, 0, scale.w, 0,
			0, 0, 0, 0, scale.v
		);
	}
	inline mat5_t<T> Transpose() const {
		return FromRows(
			Col<0>(),
			Col<1>(),
			Col<2>(),
			Col<3>(),
			Col<4>()
		);
	}
	inline mat5_t<T> operator+(mat5_t<T> a) const {
		return FromCols(
			Col<0>() + a,
			Col<1>() + a,
			Col<2>() + a,
			Col<3>() + a,
			Col<4>() + a
		);
	}
	inline mat5_t<T> operator*(mat5_t<T> rhs) const {
		return mat5_t<T>(
			dot(Row<0>(), rhs.Col<0>()), dot(Row<1>(), rhs.Col<0>()), dot(Row<2>(), rhs.Col<0>()), dot(Row<3>(), rhs.Col<0>()), dot(Row<4>(), rhs.Col<0>()),
			dot(Row<0>(), rhs.Col<1>()), dot(Row<1>(), rhs.Col<1>()), dot(Row<2>(), rhs.Col<1>()), dot(Row<3>(), rhs.Col<1>()), dot(Row<4>(), rhs.Col<1>()),
			dot(Row<0>(), rhs.Col<2>()), dot(Row<1>(), rhs.Col<2>()), dot(Row<2>(), rhs.Col<2>()), dot(Row<3>(), rhs.Col<2>()), dot(Row<4>(), rhs.Col<2>()),
			dot(Row<0>(), rhs.Col<3>()), dot(Row<1>(), rhs.Col<3>()), dot(Row<2>(), rhs.Col<3>()), dot(Row<3>(), rhs.Col<3>()), dot(Row<4>(), rhs.Col<3>()),
			dot(Row<0>(), rhs.Col<4>()), dot(Row<1>(), rhs.Col<4>()), dot(Row<2>(), rhs.Col<4>()), dot(Row<3>(), rhs.Col<4>()), dot(Row<4>(), rhs.Col<4>())
		);
	}
	inline vec5_t<T> operator*(vec5_t<T> rhs) const {
		return vec5_t<T>(
			dot(Row<0>(), rhs),
			dot(Row<1>(), rhs),
			dot(Row<2>(), rhs),
			dot(Row<3>(), rhs),
			dot(Row<4>(), rhs)
		);
	}
	inline mat5_t<T> operator*(T a) const {
		return FromCols(
			Col<0>() * a,
			Col<1>() * a,
			Col<2>() * a,
			Col<3>() * a,
			Col<4>() * a
		);
	}
	inline mat5_t<T> operator/(T a) const {
		return FromCols(
			Col<0>() / a,
			Col<1>() / a,
			Col<2>() / a,
			Col<3>() / a,
			Col<4>() / a
		);
	}
};

typedef mat5_t<f32> mat5;
typedef mat5_t<f64> mat5d;

} // namespace AzCore

template <typename T>
inline AzCore::vec5_t<T> operator*(AzCore::vec5_t<T> lhs, AzCore::mat5_t<T> rhs) {
	return AzCore::vec5_t<T>(
		dot(lhs, rhs.Col<0>()),
		dot(lhs, rhs.Col<1>()),
		dot(lhs, rhs.Col<2>()),
		dot(lhs, rhs.Col<3>()),
		dot(lhs, rhs.Col<4>())
	);
}

#endif // AZCORE_MATH_MAT5_T_HPP
