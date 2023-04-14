/*
	File: mat3_t.hpp
	Author: Philip Haynes
*/

#ifndef AZCORE_MATH_MAT3_T_HPP
#define AZCORE_MATH_MAT3_T_HPP

#include "mat2_t.hpp"
#include "vec3_t.hpp"

namespace AzCore {

// 3x3 matrix with the data in a column-major layout to match GLSL's behavior
template <typename T>
struct mat3_t {
	union {
		struct {
			T x1, y1, z1,
			  x2, y2, z2,
			  x3, y3, z3;
		} h;
		struct {
			T x1, x2, x3,
			  y1, y2, y3,
			  z1, z2, z3;
		} v;
		struct {
			T data[9];
		};
	};
	mat3_t() = default;
	inline mat3_t(mat2_t<T> in) : data{
		in.h.x1, in.h.y1, 0,
		in.h.x2, in.h.y2, 0,
		0,       0,       1
	} {}
	inline mat3_t(T a) : data{a, 0, 0, 0, a, 0, 0, 0, a} {}
	inline mat3_t(
		T x1, T y1, T z1,
		T x2, T y2, T z2,
		T x3, T y3, T z3
	) : data{
		x1, y1, z1,
		x2, y2, z2,
		x3, y3, z3
	} {}
	inline static mat3_t<T> FromCols(vec3_t<T> col1, vec3_t<T> col2, vec3_t<T> col3) {
		mat3_t<T> result(
			col1.x, col1.y, col1.z,
			col2.x, col2.y, col2.z,
			col3.x, col3.y, col3.z
		);
		return result;
	}
	inline static mat3_t<T> FromRows(vec3_t<T> row1, vec3_t<T> row2, vec3_t<T> row3) {
		mat3_t<T> result(
			row1.x, row1.y, row1.z,
			row2.x, row2.y, row2.z,
			row3.x, row3.y, row3.z
		);
		return result;
	}
	inline mat3_t(const T d[9]) : data{d[0], d[1], d[2], d[3], d[4], d[5], d[6], d[7], d[9]} {}
	inline vec3_t<T> Col1() const { return vec3_t<T>(h.x1, h.y1, h.z1); }
	inline vec3_t<T> Col2() const { return vec3_t<T>(h.x2, h.y2, h.z2); }
	inline vec3_t<T> Col3() const { return vec3_t<T>(h.x3, h.y3, h.z3); }
	inline vec3_t<T> Row1() const { return vec3_t<T>(v.x1, v.y1, v.z1); }
	inline vec3_t<T> Row2() const { return vec3_t<T>(v.x2, v.y2, v.z2); }
	inline vec3_t<T> Row3() const { return vec3_t<T>(v.x3, v.y3, v.z3); }
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
			c + xx * ic,         xy * ic - a.z * s,   xz * ic + a.y * s,
			xy * ic + a.z * s,   c + yy * ic,         yz * ic - a.x * s,
			xz * ic - a.y * s,   yz * ic + a.x * s,   c + zz * ic
		);
	}
	static mat3_t<T> Scaler(vec3_t<T> scale) {
		return mat3_t<T>(
			scale.x, 0, 0,
			0, scale.y, 0,
			0, 0, scale.z
		);
	}
	inline mat3_t<T> Transpose() const {
		return mat3_t<T>(
			v.x1, v.y1, v.z1,
			v.x2, v.y2, v.z2,
			v.x3, v.y3, v.z3
		);
	}
	inline mat3_t<T> operator+(mat3_t<T> a) const {
		return mat3_t<T>(
			h.x1 + a.h.x1, h.y1 + a.h.y1, h.z1 + a.h.z1,
			h.x2 + a.h.x2, h.y2 + a.h.y2, h.z2 + a.h.z2,
			h.x3 + a.h.x3, h.y3 + a.h.y3, h.z3 + a.h.z3
		);
	}
	inline mat3_t<T> operator*(mat3_t<T> rhs) const {
		return mat3_t<T>(
			dot(Row1(), rhs.Col1()), dot(Row2(), rhs.Col1()), dot(Row3(), rhs.Col1()),
			dot(Row1(), rhs.Col2()), dot(Row2(), rhs.Col2()), dot(Row3(), rhs.Col2()),
			dot(Row1(), rhs.Col3()), dot(Row2(), rhs.Col3()), dot(Row3(), rhs.Col3())
		);
	}
	inline vec3_t<T> operator*(vec3_t<T> rhs) const {
		return vec3_t<T>(
			dot(Row1(), rhs),
			dot(Row2(), rhs),
			dot(Row3(), rhs)
		);
	}
	inline mat3_t<T> operator*(T a) const {
		return mat3_t<T>(
			h.x1 * a, h.y1 * a, h.z1 * a,
			h.x2 * a, h.y2 * a, h.z2 * a,
			h.x3 * a, h.y3 * a, h.z3 * a
		);
	}
	inline mat3_t<T> operator/(T a) const {
		return mat3_t<T>(
			h.x1 / a, h.y1 / a, h.z1 / a,
			h.x2 / a, h.y2 / a, h.z2 / a,
			h.x3 / a, h.y3 / a, h.z3 / a
		);
	}
};

typedef mat3_t<f32> mat3;
typedef mat3_t<f64> mat3d;

} // namespace AzCore

template <typename T>
inline AzCore::vec3_t<T> operator*(AzCore::vec3_t<T> lhs, AzCore::mat3_t<T> rhs) {
	return AzCore::vec3_t<T>(
		dot(lhs, rhs.Col1()),
		dot(lhs, rhs.Col2()),
		dot(lhs, rhs.Col3())
	);
}

#endif // AZCORE_MATH_MAT3_T_HPP
