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

// 5x5 matrix with the data in a column-major layout to be consistent with the rest of the matrices
template <typename T>
struct mat5_t {
	union {
		struct {
			T x1, y1, z1, w1, v1,
			  x2, y2, z2, w2, v2,
			  x3, y3, z3, w3, v3,
			  x4, y4, z4, w4, v4,
			  x5, y5, z5, w5, v5;
		} h;
		struct {
			T x1, x2, x3, x4, x5,
			  y1, y2, y3, y4, y5,
			  z1, z2, z3, z4, z5,
			  w1, w2, w3, w4, w5,
			  v1, v2, v3, v4, v5;
		} v;
		struct {
			T data[25];
		};
	};
	mat5_t() = default;
	inline mat5_t(mat4_t<T> in) : data{
		in.h.x1, in.h.y1, in.h.z1, in.h.w1, 0,
		in.h.x2, in.h.y2, in.h.z2, in.h.w2, 0,
		in.h.x3, in.h.y3, in.h.z3, in.h.w3, 0,
		in.h.x4, in.h.y4, in.h.z4, in.h.w4, 0,
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
		T x1, T y1, T z1, T w1, T v1,
		T x2, T y2, T z2, T w2, T v2,
		T x3, T y3, T z3, T w3, T v3,
		T x4, T y4, T z4, T w4, T v4,
		T x5, T y5, T z5, T w5, T v5
	) : data{
		x1, y1, z1, w1, v1,
		x2, y2, z2, w2, v2,
		x3, y3, z3, w3, v3,
		x4, y4, z4, w4, v4,
		x5, y5, z5, w5, v5
	} {}
	inline static mat5_t<T> FromCols(vec5_t<T> col1, vec5_t<T> col2, vec5_t<T> col3, vec5_t<T> col4, vec5_t<T> col5) {
		mat5_t<T> result(
			col1.x, col1.y, col1.z, col1.w, col1.v,
			col2.x, col2.y, col2.z, col2.w, col2.v,
			col3.x, col3.y, col3.z, col3.w, col3.v,
			col4.x, col4.y, col4.z, col4.w, col4.v,
			col5.x, col5.y, col5.z, col5.w, col5.v
		);
		return result;
	}
	inline static mat5_t<T> FromRows(vec5_t<T> row1, vec5_t<T> row2, vec5_t<T> row3, vec5_t<T> row4, vec5_t<T> row5) {
		mat5_t<T> result(
			row1.x, row1.y, row1.z, row1.w, row1.v,
			row2.x, row2.y, row2.z, row2.w, row2.v,
			row3.x, row3.y, row3.z, row3.w, row3.v,
			row4.x, row4.y, row4.z, row4.w, row4.v,
			row5.x, row5.y, row5.z, row5.w, row5.v
		);
		return result;
	}
	inline mat5_t(const T d[25]) : data{
		d[0],  d[1],  d[2],  d[3],  d[4],
		d[5],  d[6],  d[7],  d[8],  d[9],
		d[10], d[11], d[12], d[13], d[14],
		d[15], d[16], d[17], d[18], d[19],
		d[20], d[21], d[22], d[23], d[24]
	} {}
	inline vec5_t<T> Col1() const { return vec5_t<T>(h.x1, h.y1, h.z1, h.w1, h.v1); }
	inline vec5_t<T> Col2() const { return vec5_t<T>(h.x2, h.y2, h.z2, h.w2, h.v2); }
	inline vec5_t<T> Col3() const { return vec5_t<T>(h.x3, h.y3, h.z3, h.w3, h.v3); }
	inline vec5_t<T> Col4() const { return vec5_t<T>(h.x4, h.y4, h.z4, h.w4, h.v4); }
	inline vec5_t<T> Col5() const { return vec5_t<T>(h.x5, h.y5, h.z5, h.w5, h.v5); }
	inline vec5_t<T> Row1() const { return vec5_t<T>(v.x1, v.y1, v.z1, v.w1, v.v1); }
	inline vec5_t<T> Row2() const { return vec5_t<T>(v.x2, v.y2, v.z2, v.w2, v.v2); }
	inline vec5_t<T> Row3() const { return vec5_t<T>(v.x3, v.y3, v.z3, v.w3, v.v3); }
	inline vec5_t<T> Row4() const { return vec5_t<T>(v.x4, v.y4, v.z4, v.w4, v.v4); }
	inline vec5_t<T> Row5() const { return vec5_t<T>(v.x5, v.y5, v.z5, v.w5, v.v5); }
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
	static mat5_t<T> Scaler(vec5_t<T> scale) {
		return mat5_t<T>(
			scale.x, 0, 0, 0, 0,
			0, scale.y, 0, 0, 0,
			0, 0, scale.z, 0, 0,
			0, 0, 0, scale.w, 0,
			0, 0, 0, 0, scale.v
		);
	}
	inline mat5_t<T> Transpose() const {
		return mat5_t<T>(
			v.x1, v.y1, v.z1, v.w1, v.v1,
			v.x2, v.y2, v.z2, v.w2, v.v2,
			v.x3, v.y3, v.z3, v.w3, v.v3,
			v.x4, v.y4, v.z4, v.w4, v.v4,
			v.x5, v.y5, v.z5, v.w5, v.v5
		);
	}
	inline mat5_t<T> operator+(mat5_t<T> a) const {
		return mat5_t<T>(
			h.x1 + a.h.x1, h.y1 + a.h.y1, h.z1 + a.h.z1, h.w1 + a.h.w1, h.v1 + a.h.v1,
			h.x2 + a.h.x2, h.y2 + a.h.y2, h.z2 + a.h.z2, h.w2 + a.h.w2, h.v2 + a.h.v2,
			h.x3 + a.h.x3, h.y3 + a.h.y3, h.z3 + a.h.z3, h.w3 + a.h.w3, h.v3 + a.h.v3,
			h.x4 + a.h.x4, h.y4 + a.h.y4, h.z4 + a.h.z4, h.w4 + a.h.w4, h.v4 + a.h.v4,
			h.x5 + a.h.x5, h.y5 + a.h.y5, h.z5 + a.h.z5, h.w5 + a.h.w5, h.v5 + a.h.v5
		);
	}
	inline mat5_t<T> operator*(mat5_t<T> rhs) const {
		return mat5_t<T>(
			dot(Row1(), rhs.Col1()), dot(Row2(), rhs.Col1()), dot(Row3(), rhs.Col1()), dot(Row4(), rhs.Col1()), dot(Row5(), rhs.Col1()),
			dot(Row1(), rhs.Col2()), dot(Row2(), rhs.Col2()), dot(Row3(), rhs.Col2()), dot(Row4(), rhs.Col2()), dot(Row5(), rhs.Col2()),
			dot(Row1(), rhs.Col3()), dot(Row2(), rhs.Col3()), dot(Row3(), rhs.Col3()), dot(Row4(), rhs.Col3()), dot(Row5(), rhs.Col3()),
			dot(Row1(), rhs.Col4()), dot(Row2(), rhs.Col4()), dot(Row3(), rhs.Col4()), dot(Row4(), rhs.Col4()), dot(Row5(), rhs.Col4()),
			dot(Row1(), rhs.Col5()), dot(Row2(), rhs.Col5()), dot(Row3(), rhs.Col5()), dot(Row4(), rhs.Col5()), dot(Row5(), rhs.Col5())
		);
	}
	inline vec5_t<T> operator*(vec5_t<T> rhs) const {
		return vec5_t<T>(
			dot(Row1(), rhs),
			dot(Row2(), rhs),
			dot(Row3(), rhs),
			dot(Row4(), rhs),
			dot(Row5(), rhs)
		);
	}
	inline mat5_t<T> operator*(T a) const {
		return mat5_t<T>(
			h.x1 * a, h.y1 * a, h.z1 * a, h.w1 * a, h.v1 * a,
			h.x2 * a, h.y2 * a, h.z2 * a, h.w2 * a, h.v2 * a,
			h.x3 * a, h.y3 * a, h.z3 * a, h.w3 * a, h.v3 * a,
			h.x4 * a, h.y4 * a, h.z4 * a, h.w4 * a, h.v4 * a,
			h.x5 * a, h.y5 * a, h.z5 * a, h.w5 * a, h.v5 * a
		);
	}
	inline mat5_t<T> operator/(T a) const {
		return mat5_t<T>(
			h.x1 / a, h.y1 / a, h.z1 / a, h.w1 / a, h.v1 / a,
			h.x2 / a, h.y2 / a, h.z2 / a, h.w2 / a, h.v2 / a,
			h.x3 / a, h.y3 / a, h.z3 / a, h.w3 / a, h.v3 / a,
			h.x4 / a, h.y4 / a, h.z4 / a, h.w4 / a, h.v4 / a,
			h.x5 / a, h.y5 / a, h.z5 / a, h.w5 / a, h.v5 / a);
	}
};

typedef mat5_t<f32> mat5;
typedef mat5_t<f64> mat5d;

} // namespace AzCore

template <typename T>
inline AzCore::vec5_t<T> operator*(AzCore::vec5_t<T> lhs, AzCore::mat5_t<T> rhs) {
	return AzCore::vec5_t<T>(
		dot(lhs, rhs.Col1()),
		dot(lhs, rhs.Col2()),
		dot(lhs, rhs.Col3()),
		dot(lhs, rhs.Col4()),
		dot(lhs, rhs.Col5())
	);
}

#endif // AZCORE_MATH_MAT5_T_HPP
