/*
	File: mat4_t.hpp
	Author: Philip Haynes
*/

#ifndef AZCORE_MATH_MAT4_T_HPP
#define AZCORE_MATH_MAT4_T_HPP

#include "vec4_t.hpp"
#include "mat3_t.hpp"
#include "Angle.hpp"
#include "../Assert.hpp"
#include "../Memory/String.hpp"

namespace AzCore {

// 4x4 matrix with the conventions matching GLSL
// - column-major memory layout
// - post-multiplication (transforms are applied in right-to-left order)
// - multiplication means lhs rows are dotted with rhs columns
// - vectors are row vectors on the lhs, and column vectors on the rhs
// A typical Translation Rotation Scale setup would be applied as such: T*R*S*v
template <typename T>
struct mat4_t {
	union {
		vec4_t<T> cols[4];
		T data[16];
	};
	mat4_t() = default;
	inline mat4_t(mat3_t<T> in) : data{
		in.cols[0][0], in.cols[0][1], in.cols[0][2], 0,
		in.cols[1][0], in.cols[1][1], in.cols[1][2], 0,
		in.cols[2][0], in.cols[2][1], in.cols[2][2], 0,
		0            , 0            , 0            , 1
	} {}
	inline mat4_t(T a) : data{
		a, 0, 0, 0,
		0, a, 0, 0,
		0, 0, a, 0,
		0, 0, 0, a
	} {}
	inline mat4_t(
		T col_0_x, T col_0_y, T col_0_z, T col_0_w,
		T col_1_x, T col_1_y, T col_1_z, T col_1_w,
		T col_2_x, T col_2_y, T col_2_z, T col_2_w,
		T col_3_x, T col_3_y, T col_3_z, T col_3_w
	) : data{
		col_0_x, col_0_y, col_0_z, col_0_w,
		col_1_x, col_1_y, col_1_z, col_1_w,
		col_2_x, col_2_y, col_2_z, col_2_w,
		col_3_x, col_3_y, col_3_z, col_3_w
	} {}
	inline static mat4_t<T> FromCols(const vec4_t<T> &col_0, const vec4_t<T> &col_1, const vec4_t<T> &col_2, const vec4_t<T> &col_3) {
		mat4_t<T> result(
			col_0.x, col_0.y, col_0.z, col_0.w,
			col_1.x, col_1.y, col_1.z, col_1.w,
			col_2.x, col_2.y, col_2.z, col_2.w,
			col_3.x, col_3.y, col_3.z, col_3.w
		);
		return result;
	}
	inline static mat4_t<T> FromRows(const vec4_t<T> &row_0, const vec4_t<T> &row_1, const vec4_t<T> &row_2, const vec4_t<T> &row_3) {
		mat4_t<T> result(
			row_0.x, row_1.x, row_2.x, row_3.x,
			row_0.y, row_1.y, row_2.y, row_3.y,
			row_0.z, row_1.z, row_2.z, row_3.z,
			row_0.w, row_1.w, row_2.w, row_3.w
		);
		return result;
	}
	inline mat4_t(const T d[16]) : data{d[0], d[1], d[2], d[3], d[4], d[5], d[6], d[7], d[9], d[10], d[11], d[12], d[13], d[14], d[15]} {}
	inline vec4_t<T>& operator[](i32 column) {
		AzAssert(column >= 0 && column < 4, Stringify("Invalid column (", column, ") in mat4_t::operator[]"));
		return cols[column];
	}
	inline const vec4_t<T>& operator[](i32 column) const {
		AzAssert(column >= 0 && column < 4, Stringify("Invalid column (", column, ") in mat4_t::operator[]"));
		return cols[column];
	}
	template<i32 col>
	inline vec4_t<T> Col() const {
		static_assert(col >= 0 && col < 4);
		return cols[col];
	}
	template<i32 row>
	inline vec4_t<T> Row() const {
		static_assert(row >= 0 && row < 4);
		return vec4_t<T>(cols[0][row], cols[1][row], cols[2][row], cols[3][row]);
	}
	// Returns the 3x3 sub-matrix in the lower 3 rows and columns
	inline mat3_t<T> TrimmedMat3() const {
		mat3_t<T> result = mat3_t<T>::FromCols(
			cols[0].xyz,
			cols[1].xyz,
			cols[2].xyz
		);
		return result;
	}
	inline static mat4_t<T> Identity() {
		return mat4_t(1);
	};
	// Only useful for rotations about aligned planes, such as {{1, 0, 0, 0}, {0, 0, 0, 1}}
	// Note: The planes stay fixed in place and everything else rotates around them.
	static mat4_t<T> RotationBasic(T angle, Plane plane) {
		T s = sin(angle), c = cos(angle);
		switch (plane) {
		case Plane::XW:
			return mat4_t<T>(
				1, 0, 0, 0,
				0, c, s, 0,
				0,-s, c, 0,
				0, 0, 0, 1
			);
		case Plane::YW:
			return mat4_t<T>(
				c, 0,-s, 0,
				0, 1, 0, 0,
				s, 0, c, 0,
				0, 0, 0, 1
			);
		case Plane::ZW:
			return mat4_t<T>(
				 c, s, 0, 0,
				-s, c, 0, 0,
				 0, 0, 1, 0,
				 0, 0, 0, 1
			);
		case Plane::XY:
			return mat4_t<T>(
				1, 0, 0, 0,
				0, 1, 0, 0,
				0, 0, c, s,
				0, 0,-s, c
			);
		case Plane::YZ:
			return mat4_t<T>(
				 c, 0, 0, s,
				 0, 1, 0, 0,
				 0, 0, 1, 0,
				-s, 0, 0, c
			);
		case Plane::ZX:
			return mat4_t<T>(
				1, 0, 0, 0,
				0, c, 0,-s,
				0, 0, 1, 0,
				0, s, 0, c
			);
		}
		return mat4_t<T>();
	}
	// For using 3D-axis rotations
	static mat4_t<T> RotationBasic(T angle, Axis axis) {
		switch (axis) {
		case Axis::X:
			return RotationBasic(angle, Plane::XW);
		case Axis::Y:
			return RotationBasic(angle, Plane::YW);
		case Axis::Z:
			return RotationBasic(angle, Plane::ZW);
		}
		return mat4_t<T>();
	}
	// Useful for arbitrary 3D-axes
	static mat4_t<T> Rotation(T angle, const vec3_t<T> &axis) {
		T s = sin(angle), c = cos(angle);
		T ic = 1 - c;
		vec3_t<T> a = normalize(axis);
		T xx = square(a.x), yy = square(a.y), zz = square(a.z),
		  xy = a.x * a.y, xz = a.x * a.z, yz = a.y * a.z;
		return mat4_t<T>(
			c + xx * ic,         xy * ic + a.z * s,   xz * ic - a.y * s,   0,
			xy * ic - a.z * s,   c + yy * ic,         yz * ic + a.x * s,   0,
			xz * ic + a.y * s,   yz * ic - a.x * s,   c + zz * ic,         0,
			0,                   0,                   0,                   1
		);
	}
	static mat4_t<T> Scale(const vec4_t<T> &scale) {
		return mat4_t<T>(
			scale.x, 0, 0, 0,
			0, scale.y, 0, 0,
			0, 0, scale.z, 0,
			0, 0, 0, scale.w
		);
	}

	mat4_t<T> Inverse() const {
		T A2323 = cols[2][2] * cols[3][3] - cols[3][2] * cols[2][3];
		T A1323 = cols[1][2] * cols[3][3] - cols[3][2] * cols[1][3];
		T A1223 = cols[1][2] * cols[2][3] - cols[2][2] * cols[1][3];
		T A0323 = cols[0][2] * cols[3][3] - cols[3][2] * cols[0][3];
		T A0223 = cols[0][2] * cols[2][3] - cols[2][2] * cols[0][3];
		T A0123 = cols[0][2] * cols[1][3] - cols[1][2] * cols[0][3];
		T A2313 = cols[2][1] * cols[3][3] - cols[3][1] * cols[2][3];
		T A1313 = cols[1][1] * cols[3][3] - cols[3][1] * cols[1][3];
		T A1213 = cols[1][1] * cols[2][3] - cols[2][1] * cols[1][3];
		T A2312 = cols[2][1] * cols[3][2] - cols[3][1] * cols[2][2];
		T A1312 = cols[1][1] * cols[3][2] - cols[3][1] * cols[1][2];
		T A1212 = cols[1][1] * cols[2][2] - cols[2][1] * cols[1][2];
		T A0313 = cols[0][1] * cols[3][3] - cols[3][1] * cols[0][3];
		T A0213 = cols[0][1] * cols[2][3] - cols[2][1] * cols[0][3];
		T A0312 = cols[0][1] * cols[3][2] - cols[3][1] * cols[0][2];
		T A0212 = cols[0][1] * cols[2][2] - cols[2][1] * cols[0][2];
		T A0113 = cols[0][1] * cols[1][3] - cols[1][1] * cols[0][3];
		T A0112 = cols[0][1] * cols[1][2] - cols[1][1] * cols[0][2];

		T det = cols[0][0] * (cols[1][1] * A2323 - cols[2][1] * A1323 + cols[3][1] * A1223)
		      - cols[1][0] * (cols[0][1] * A2323 - cols[2][1] * A0323 + cols[3][1] * A0223)
		      + cols[2][0] * (cols[0][1] * A1323 - cols[1][1] * A0323 + cols[3][1] * A0123)
		      - cols[3][0] * (cols[0][1] * A1223 - cols[1][1] * A0223 + cols[2][1] * A0123);
		det = 1 / det;

		mat4_t<T> result;
		result.cols[0][0] = det *  (cols[1][1] * A2323 - cols[2][1] * A1323 + cols[3][1] * A1223);
		result.cols[1][0] = det * -(cols[1][0] * A2323 - cols[2][0] * A1323 + cols[3][0] * A1223);
		result.cols[2][0] = det *  (cols[1][0] * A2313 - cols[2][0] * A1313 + cols[3][0] * A1213);
		result.cols[3][0] = det * -(cols[1][0] * A2312 - cols[2][0] * A1312 + cols[3][0] * A1212);
		result.cols[0][1] = det * -(cols[0][1] * A2323 - cols[2][1] * A0323 + cols[3][1] * A0223);
		result.cols[1][1] = det *  (cols[0][0] * A2323 - cols[2][0] * A0323 + cols[3][0] * A0223);
		result.cols[2][1] = det * -(cols[0][0] * A2313 - cols[2][0] * A0313 + cols[3][0] * A0213);
		result.cols[3][1] = det *  (cols[0][0] * A2312 - cols[2][0] * A0312 + cols[3][0] * A0212);
		result.cols[0][2] = det *  (cols[0][1] * A1323 - cols[1][1] * A0323 + cols[3][1] * A0123);
		result.cols[1][2] = det * -(cols[0][0] * A1323 - cols[1][0] * A0323 + cols[3][0] * A0123);
		result.cols[2][2] = det *  (cols[0][0] * A1313 - cols[1][0] * A0313 + cols[3][0] * A0113);
		result.cols[3][2] = det * -(cols[0][0] * A1312 - cols[1][0] * A0312 + cols[3][0] * A0112);
		result.cols[0][3] = det * -(cols[0][1] * A1223 - cols[1][1] * A0223 + cols[2][1] * A0123);
		result.cols[1][3] = det *  (cols[0][0] * A1223 - cols[1][0] * A0223 + cols[2][0] * A0123);
		result.cols[2][3] = det * -(cols[0][0] * A1213 - cols[1][0] * A0213 + cols[2][0] * A0113);
		result.cols[3][3] = det *  (cols[0][0] * A1212 - cols[1][0] * A0212 + cols[2][0] * A0112);
		return result;
	}

	// Much faster than Inverse(), but only applicable to Transforms with only Rotation and Translation
	mat4_t<T> InverseUnscaledTransform() const {
		mat3_t<T> rot = TrimmedMat3().Transpose();
		mat4_t<T> result = mat4_t<T>(rot);
		result[3].xyz = -(rot * cols[3].xyz);
		return result;
	}


	inline mat4_t<T> Transpose() const {
		return FromRows(
			Col<0>(),
			Col<1>(),
			Col<2>(),
			Col<3>()
		);
	}
	inline mat4_t<T> operator+(const mat4_t<T> &rhs) const {
		return FromCols(
			Col<0>() + rhs.template Col<0>(),
			Col<1>() + rhs.template Col<1>(),
			Col<2>() + rhs.template Col<2>(),
			Col<3>() + rhs.template Col<3>()
		);
	}
	inline mat4_t<T> operator*(const mat4_t<T> &rhs) const {
		return mat4_t<T>(
			dot(Row<0>(), rhs.template Col<0>()), dot(Row<1>(), rhs.template Col<0>()), dot(Row<2>(), rhs.template Col<0>()), dot(Row<3>(), rhs.template Col<0>()),
			dot(Row<0>(), rhs.template Col<1>()), dot(Row<1>(), rhs.template Col<1>()), dot(Row<2>(), rhs.template Col<1>()), dot(Row<3>(), rhs.template Col<1>()),
			dot(Row<0>(), rhs.template Col<2>()), dot(Row<1>(), rhs.template Col<2>()), dot(Row<2>(), rhs.template Col<2>()), dot(Row<3>(), rhs.template Col<2>()),
			dot(Row<0>(), rhs.template Col<3>()), dot(Row<1>(), rhs.template Col<3>()), dot(Row<2>(), rhs.template Col<3>()), dot(Row<3>(), rhs.template Col<3>())
		);
	}
	inline vec4_t<T> operator*(const vec4_t<T> &rhs) const {
		return vec4_t<T>(
			dot(Row<0>(), rhs),
			dot(Row<1>(), rhs),
			dot(Row<2>(), rhs),
			dot(Row<3>(), rhs)
		);
	}
	inline mat4_t<T> operator*(T a) const {
		return FromCols(
			Col<0>() * a,
			Col<1>() * a,
			Col<2>() * a,
			Col<3>() * a
		);
	}
	inline mat4_t<T> operator/(T a) const {
		return FromCols(
			Col<0>() / a,
			Col<1>() / a,
			Col<2>() / a,
			Col<3>() / a
		);
	}
	// forward must be a unit vector
	static mat4_t<T> Camera(vec3_t<T> pos, vec3_t<T> forward, vec3_t<T> up=vec3_t<T>(0, 0, 1));
	static mat4_t<T> Perspective(Radians<T> fovX, T widthOverHeight, T nearClip, T farClip);
	static mat4_t<T> Ortho(T width, T height, T nearClip, T farClip);
};
extern template mat4_t<f32> mat4_t<f32>::Camera(vec3, vec3, vec3);
extern template mat4_t<f32> mat4_t<f32>::Perspective(Radians32, f32, f32, f32);
extern template mat4_t<f64> mat4_t<f64>::Camera(vec3d, vec3d, vec3d);
extern template mat4_t<f64> mat4_t<f64>::Perspective(Radians64, f64, f64, f64);

typedef mat4_t<f32> mat4;
typedef mat4_t<f64> mat4d;

typedef mat4_t<f32> mat4;
typedef mat4_t<f64> mat4d;

} // namespace AzCore

template <typename T>
inline AzCore::vec4_t<T> operator*(const AzCore::vec4_t<T> &lhs, const AzCore::mat4_t<T> &rhs) {
	return AzCore::vec4_t<T>(
		dot(lhs, rhs.template Col<0>()),
		dot(lhs, rhs.template Col<1>()),
		dot(lhs, rhs.template Col<2>()),
		dot(lhs, rhs.template Col<3>())
	);
}

#endif // AZCORE_MATH_MAT4_T_HPP
