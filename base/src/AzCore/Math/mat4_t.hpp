/*
	File: mat4_t.hpp
	Author: Philip Haynes
*/

#ifndef AZCORE_MATH_MAT4_T_HPP
#define AZCORE_MATH_MAT4_T_HPP

#include "vec4_t.hpp"
#include "mat3_t.hpp"
#include "Angle.hpp"

namespace AzCore {

// 4x4 matrix with the data in a column-major layout to match GLSL's behavior
template <typename T>
struct mat4_t {
	union {
		struct {
			T x1, y1, z1, w1,
			  x2, y2, z2, w2,
			  x3, y3, z3, w3,
			  x4, y4, z4, w4;
		} h;
		struct {
			T x1, x2, x3, x4,
			  y1, y2, y3, y4,
			  z1, z2, z3, z4,
			  w1, w2, w3, w4;
		} v;
		struct {
			T data[16];
		};
	};
	mat4_t() = default;
	inline mat4_t(mat3_t<T> in) : data{
		in.h.x1, in.h.y1, in.h.z1, 0,
		in.h.x2, in.h.y2, in.h.z2, 0,
		in.h.x3, in.h.y3, in.h.z3, 0,
		0,       0,       0,       1
	} {}
	inline mat4_t(T a) : data{
		a, 0, 0, 0,
		0, a, 0, 0,
		0, 0, a, 0,
		0, 0, 0, a
	} {}
	inline mat4_t(
		T x1, T y1, T z1, T w1,
		T x2, T y2, T z2, T w2,
		T x3, T y3, T z3, T w3,
		T x4, T y4, T z4, T w4
	) : data{
		x1, y1, z1, w1,
		x2, y2, z2, w2,
		x3, y3, z3, w3,
		x4, y4, z4, w4
	} {}
	inline static mat4_t<T> FromCols(vec4_t<T> col1, vec4_t<T> col2, vec4_t<T> col3, vec4_t<T> col4) {
		mat4_t<T> result(
			col1.x, col1.y, col1.z, col1.w,
			col2.x, col2.y, col2.z, col2.w,
			col3.x, col3.y, col3.z, col3.w,
			col4.x, col4.y, col4.z, col4.w
		);
		return result;
	}
	inline static mat4_t<T> FromRows(vec4_t<T> row1, vec4_t<T> row2, vec4_t<T> row3, vec4_t<T> row4) {
		mat4_t<T> result(
			row1.x, row1.y, row1.z, row1.w,
			row2.x, row2.y, row2.z, row2.w,
			row3.x, row3.y, row3.z, row3.w,
			row4.x, row4.y, row4.z, row4.w
		);
		return result;
	}
	inline mat4_t(const T d[16]) : data{d[0], d[1], d[2], d[3], d[4], d[5], d[6], d[7], d[9], d[10], d[11], d[12], d[13], d[14], d[15]} {}
	inline vec4_t<T> Col1() const { return vec4_t<T>(h.x1, h.y1, h.z1, h.w1); }
	inline vec4_t<T> Col2() const { return vec4_t<T>(h.x2, h.y2, h.z2, h.w2); }
	inline vec4_t<T> Col3() const { return vec4_t<T>(h.x3, h.y3, h.z3, h.w3); }
	inline vec4_t<T> Col4() const { return vec4_t<T>(h.x4, h.y4, h.z4, h.w4); }
	inline vec4_t<T> Row1() const { return vec4_t<T>(v.x1, v.y1, v.z1, v.w1); }
	inline vec4_t<T> Row2() const { return vec4_t<T>(v.x2, v.y2, v.z2, v.w2); }
	inline vec4_t<T> Row3() const { return vec4_t<T>(v.x3, v.y3, v.z3, v.w3); }
	inline vec4_t<T> Row4() const { return vec4_t<T>(v.x4, v.y4, v.z4, v.w4); }
	inline static mat4_t<T> Identity() {
		return mat4_t(1);
	};
	// Only useful for rotations about aligned planes, such as {{1, 0, 0, 0}, {0, 0, 0, 1}}
	// Note: The planes stay fixed in place and everything else rotates around the
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
	static mat4_t<T> Rotation(T angle, vec3_t<T> axis) {
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
	static mat4_t<T> Scaler(vec4_t<T> scale) {
		return mat4_t<T>(
			scale.x, 0, 0, 0,
			0, scale.y, 0, 0,
			0, 0, scale.z, 0,
			0, 0, 0, scale.w
		);
	}
	
	mat4_t<T> Inverse() const {
		T A2323 = h.z3 * h.w4 - h.z4 * h.w3;
		T A1323 = h.z2 * h.w4 - h.z4 * h.w2;
		T A1223 = h.z2 * h.w3 - h.z3 * h.w2;
		T A0323 = h.z1 * h.w4 - h.z4 * h.w1;
		T A0223 = h.z1 * h.w3 - h.z3 * h.w1;
		T A0123 = h.z1 * h.w2 - h.z2 * h.w1;
		T A2313 = h.y3 * h.w4 - h.y4 * h.w3;
		T A1313 = h.y2 * h.w4 - h.y4 * h.w2;
		T A1213 = h.y2 * h.w3 - h.y3 * h.w2;
		T A2312 = h.y3 * h.z4 - h.y4 * h.z3;
		T A1312 = h.y2 * h.z4 - h.y4 * h.z2;
		T A1212 = h.y2 * h.z3 - h.y3 * h.z2;
		T A0313 = h.y1 * h.w4 - h.y4 * h.w1;
		T A0213 = h.y1 * h.w3 - h.y3 * h.w1;
		T A0312 = h.y1 * h.z4 - h.y4 * h.z1;
		T A0212 = h.y1 * h.z3 - h.y3 * h.z1;
		T A0113 = h.y1 * h.w2 - h.y2 * h.w1;
		T A0112 = h.y1 * h.z2 - h.y2 * h.z1;

		T det = h.x1 * (h.y2 * A2323 - h.y3 * A1323 + h.y4 * A1223)
		      - h.x2 * (h.y1 * A2323 - h.y3 * A0323 + h.y4 * A0223)
		      + h.x3 * (h.y1 * A1323 - h.y2 * A0323 + h.y4 * A0123)
		      - h.x4 * (h.y1 * A1223 - h.y2 * A0223 + h.y3 * A0123);
		det = 1 / det;
		
		mat4_t<T> result;
		result.h.x1 = det *  (h.y2 * A2323 - h.y3 * A1323 + h.y4 * A1223);
		result.h.x2 = det * -(h.x2 * A2323 - h.x3 * A1323 + h.x4 * A1223);
		result.h.x3 = det *  (h.x2 * A2313 - h.x3 * A1313 + h.x4 * A1213);
		result.h.x4 = det * -(h.x2 * A2312 - h.x3 * A1312 + h.x4 * A1212);
		result.h.y1 = det * -(h.y1 * A2323 - h.y3 * A0323 + h.y4 * A0223);
		result.h.y2 = det *  (h.x1 * A2323 - h.x3 * A0323 + h.x4 * A0223);
		result.h.y3 = det * -(h.x1 * A2313 - h.x3 * A0313 + h.x4 * A0213);
		result.h.y4 = det *  (h.x1 * A2312 - h.x3 * A0312 + h.x4 * A0212);
		result.h.z1 = det *  (h.y1 * A1323 - h.y2 * A0323 + h.y4 * A0123);
		result.h.z2 = det * -(h.x1 * A1323 - h.x2 * A0323 + h.x4 * A0123);
		result.h.z3 = det *  (h.x1 * A1313 - h.x2 * A0313 + h.x4 * A0113);
		result.h.z4 = det * -(h.x1 * A1312 - h.x2 * A0312 + h.x4 * A0112);
		result.h.w1 = det * -(h.y1 * A1223 - h.y2 * A0223 + h.y3 * A0123);
		result.h.w2 = det *  (h.x1 * A1223 - h.x2 * A0223 + h.x3 * A0123);
		result.h.w3 = det * -(h.x1 * A1213 - h.x2 * A0213 + h.x3 * A0113);
		result.h.w4 = det *  (h.x1 * A1212 - h.x2 * A0212 + h.x3 * A0112);
		return result;
	}


	inline mat4_t<T> Transpose() const {
		return mat4_t<T>(
			v.x1, v.y1, v.z1, v.w1,
			v.x2, v.y2, v.z2, v.w2,
			v.x3, v.y3, v.z3, v.w3,
			v.x4, v.y4, v.z4, v.w4
		);
	}
	inline mat4_t<T> operator+(mat4_t<T> rhs) const {
		return mat4_t<T>(
			h.x1 + rhs.h.x1, h.y1 + rhs.h.y1, h.z1 + rhs.h.z1, h.w1 + rhs.h.w1,
			h.x2 + rhs.h.x2, h.y2 + rhs.h.y2, h.z2 + rhs.h.z2, h.w2 + rhs.h.w2,
			h.x3 + rhs.h.x3, h.y3 + rhs.h.y3, h.z3 + rhs.h.z3, h.w3 + rhs.h.w3,
			h.x4 + rhs.h.x4, h.y4 + rhs.h.y4, h.z4 + rhs.h.z4, h.w4 + rhs.h.w4
		);
	}
	inline mat4_t<T> operator*(mat4_t<T> rhs) const {
		return mat4_t<T>(
			dot(Row1(), rhs.Col1()), dot(Row2(), rhs.Col1()), dot(Row3(), rhs.Col1()), dot(Row4(), rhs.Col1()),
			dot(Row1(), rhs.Col2()), dot(Row2(), rhs.Col2()), dot(Row3(), rhs.Col2()), dot(Row4(), rhs.Col2()),
			dot(Row1(), rhs.Col3()), dot(Row2(), rhs.Col3()), dot(Row3(), rhs.Col3()), dot(Row4(), rhs.Col3()),
			dot(Row1(), rhs.Col4()), dot(Row2(), rhs.Col4()), dot(Row3(), rhs.Col4()), dot(Row4(), rhs.Col4())
		);
	}
	inline vec4_t<T> operator*(vec4_t<T> rhs) const {
		return vec4_t<T>(
			dot(Row1(), rhs),
			dot(Row2(), rhs),
			dot(Row3(), rhs),
			dot(Row4(), rhs)
		);
	}
	inline mat4_t<T> operator*(T a) const {
		return mat4_t<T>(
			h.x1 * a, h.y1 * a, h.z1 * a, h.w1 * a,
			h.x2 * a, h.y2 * a, h.z2 * a, h.w2 * a,
			h.x3 * a, h.y3 * a, h.z3 * a, h.w3 * a,
			h.x4 * a, h.y4 * a, h.z4 * a, h.w4 * a
		);
	}
	inline mat4_t<T> operator/(T a) const {
		return mat4_t<T>(
			h.x1 / a, h.y1 / a, h.z1 / a, h.w1 / a,
			h.x2 / a, h.y2 / a, h.z2 / a, h.w2 / a,
			h.x3 / a, h.y3 / a, h.z3 / a, h.w3 / a,
			h.x4 / a, h.y4 / a, h.z4 / a, h.w4 / a
		);
	}
	// forward must be a unit vector
	static mat4_t<T> Camera(vec3_t<T> pos, vec3_t<T> forward, vec3_t<T> up=vec3_t<T>(0, 0, 1));
	static mat4_t<T> Perspective(Radians<T> fovX, T widthOverHeight, T nearClip, T farClip);
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
inline AzCore::vec4_t<T> operator*(AzCore::vec4_t<T> lhs, AzCore::mat4_t<T> rhs) {
	return AzCore::vec4_t<T>(
		dot(lhs, rhs.Col1()),
		dot(lhs, rhs.Col2()),
		dot(lhs, rhs.Col3()),
		dot(lhs, rhs.Col4())
	);
}

#endif // AZCORE_MATH_MAT4_T_HPP
