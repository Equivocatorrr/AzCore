/*
	File: mat5_t.hpp
	Author: Philip Haynes
*/

#ifndef AZCORE_MATH_MAT5_T_HPP
#define AZCORE_MATH_MAT5_T_HPP

#include "vec3_t.hpp"
#include "vec5_t.hpp"

namespace AzCore {

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
	inline mat5_t(T a) : h{a, 0, 0, 0, 0, 0, a, 0, 0, 0, 0, 0, a, 0, 0, 0, 0, 0, a, 0, 0, 0, 0, 0, a} {}
	inline mat5_t(T x1, T y1, T z1, T w1, T v1,
				  T x2, T y2, T z2, T w2, T v2,
				  T x3, T y3, T z3, T w3, T v3,
				  T x4, T y4, T z4, T w4, T v4,
				  T x5, T y5, T z5, T w5, T v5)
				: data{x1, y1, z1, w1, v1, x2, y2, z2, w2, v2, x3, y3, z3, w3, v3, x4, y4, z4, w4, v4, x5, y5, z5, w5, v5} {}
	template <bool rowMajor = true>
	inline mat5_t(vec5_t<T> a, vec5_t<T> b, vec5_t<T> c, vec5_t<T> d, vec5_t<T> e) {
		if constexpr (rowMajor) {
			h = {a.x, a.y, a.z, a.w, a.v,
				 b.x, b.y, b.z, b.w, b.v,
				 c.x, c.y, c.z, c.w, c.v,
				 d.x, d.y, d.z, d.w, d.v,
				 e.x, e.y, e.z, e.w, e.v};
		} else {
			h = {a.x, b.x, c.x, d.x, e.x,
				 a.y, b.y, c.y, d.y, e.y,
				 a.z, b.z, c.z, d.z, e.z,
				 a.w, b.w, c.w, d.w, e.w,
				 a.v, b.v, c.v, d.v, e.v};
		}
	}
	inline mat5_t(const T d[25]) : data{d[0], d[1], d[2], d[3], d[4], d[5], d[6], d[7], d[9], d[10], d[11], d[12], d[13],
										d[14], d[15], d[16], d[17], d[18], d[19], d[20], d[21], d[22], d[23], d[24]} {}
	inline vec5_t<T> Row1() const { return vec5_t<T>(h.x1, h.y1, h.z1, h.w1, h.v1); }
	inline vec5_t<T> Row2() const { return vec5_t<T>(h.x2, h.y2, h.z2, h.w2, h.v2); }
	inline vec5_t<T> Row3() const { return vec5_t<T>(h.x3, h.y3, h.z3, h.w3, h.v3); }
	inline vec5_t<T> Row4() const { return vec5_t<T>(h.x4, h.y4, h.z4, h.w4, h.v4); }
	inline vec5_t<T> Row5() const { return vec5_t<T>(h.x5, h.y5, h.z5, h.w5, h.v5); }
	inline vec5_t<T> Col1() const { return vec5_t<T>(v.x1, v.y1, v.z1, v.w1, v.v1); }
	inline vec5_t<T> Col2() const { return vec5_t<T>(v.x2, v.y2, v.z2, v.w2, v.v2); }
	inline vec5_t<T> Col3() const { return vec5_t<T>(v.x3, v.y3, v.z3, v.w3, v.v3); }
	inline vec5_t<T> Col4() const { return vec5_t<T>(v.x4, v.y4, v.z4, v.w4, v.v4); }
	inline vec5_t<T> Col5() const { return vec5_t<T>(v.x5, v.y5, v.z5, v.w5, v.v5); }
	inline static mat5_t<T> Identity() {
		return mat5_t(1);
	};
	// Only useful for rotations about aligned planes, such as {{1, 0, 0, 0}, {0, 0, 0, 1}}
	// Note: The planes stay fixed in place and everything else rotates around them.
	// Note: The V-Dimension is always fixed in place
	static mat5_t<T> RotationBasic(T angle, Plane plane) {
		T s = sin(angle), c = cos(angle);
		switch (plane) {
		case Plane::XW: {
			return mat5_t<T>(
				T(1), T(0), T(0), T(0), T(0),
				T(0), c, -s, T(0), T(0),
				T(0), s, c, T(0), T(0),
				T(0), T(0), T(0), T(1), T(0),
				T(0), T(0), T(0), T(0), T(1));
		}
		case Plane::YW: {
			return mat5_t<T>(
				c, T(0), s, T(0), T(0),
				T(0), T(1), T(0), T(0), T(0),
				-s, T(0), c, T(0), T(0),
				T(0), T(0), T(0), T(1), T(0),
				T(0), T(0), T(0), T(0), T(1));
		}
		case Plane::ZW: {
			return mat5_t<T>(
				c, -s, T(0), T(0), T(0),
				s, c, T(0), T(0), T(0),
				T(0), T(0), T(1), T(0), T(0),
				T(0), T(0), T(0), T(1), T(0),
				T(0), T(0), T(0), T(0), T(1));
		}
		case Plane::XY: {
			return mat5_t<T>(
				T(1), T(0), T(0), T(0), T(0),
				T(0), T(1), T(0), T(0), T(0),
				T(0), T(0), c, -s, T(0),
				T(0), T(0), s, c, T(0),
				T(0), T(0), T(0), T(0), T(1));
		}
		case Plane::YZ: {
			return mat5_t<T>(
				c, T(0), T(0), -s, T(0),
				T(0), T(1), T(0), T(0), T(0),
				T(0), T(0), T(1), T(0), T(0),
				s, T(0), T(0), c, T(0),
				T(0), T(0), T(0), T(0), T(1));
		}
		case Plane::ZX: {
			return mat5_t<T>(
				T(1), T(0), T(0), T(0), T(0),
				T(0), c, T(0), s, T(0),
				T(0), T(0), T(1), T(0), T(0),
				T(0), -s, T(0), c, T(0),
				T(0), T(0), T(0), T(0), T(1));
		}
		}
		return mat5_t<T>();
	}
	// For using 3D-axis rotations
	static mat5_t<T> RotationBasic(T angle, Axis axis) {
		switch (axis) {
		case Axis::X: {
			return RotationBasic(angle, Plane::XW);
		}
		case Axis::Y: {
			return RotationBasic(angle, Plane::YW);
		}
		case Axis::Z: {
			return RotationBasic(angle, Plane::ZW);
		}
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
			c + xx * ic, xy * ic - a.z * s, xz * ic + a.y * s, T(0), T(0),
			xy * ic + a.z * s, c + yy * ic, yz * ic - a.x * s, T(0), T(0),
			xz * ic - a.y * s, yz * ic + a.x * s, c + zz * ic, T(0), T(0),
			T(0), T(0), T(0), T(1), T(0),
			T(0), T(0), T(0), T(0), T(1));
	}
	static mat5_t<T> Scaler(vec5_t<T> scale) {
		return mat5_t<T>(scale.x, T(0), T(0), T(0), T(0), T(0), scale.y, T(0), T(0), T(0), T(0), T(0), scale.z, T(0), T(0), T(0), T(0), T(0), scale.w, T(0), T(0), T(0), T(0), T(0), scale.v);
	}
	inline mat5_t<T> Transpose() const {
		return mat5_t<T>(v.x1, v.y1, v.z1, v.w1, v.v1,
						 v.x2, v.y2, v.z2, v.w2, v.v2,
						 v.x3, v.y3, v.z3, v.w3, v.v3,
						 v.x4, v.y4, v.z4, v.w4, v.v4,
						 v.x5, v.y5, v.z5, v.w5, v.v5);
	}
	inline mat5_t<T> operator+(mat5_t<T> a) const {
		return mat5_t<T>(
			h.x1 + a.h.x1, h.y1 + a.h.y1, h.z1 + a.h.z1, h.w1 + a.h.w1, h.v1 + a.h.v1,
			h.x2 + a.h.x2, h.y2 + a.h.y2, h.z2 + a.h.z2, h.w2 + a.h.w2, h.v2 + a.h.v2,
			h.x3 + a.h.x3, h.y3 + a.h.y3, h.z3 + a.h.z3, h.w3 + a.h.w3, h.v3 + a.h.v3,
			h.x4 + a.h.x4, h.y4 + a.h.y4, h.z4 + a.h.z4, h.w4 + a.h.w4, h.v4 + a.h.v4,
			h.x5 + a.h.x5, h.y5 + a.h.y5, h.z5 + a.h.z5, h.w5 + a.h.w5, h.v5 + a.h.v5);
	}
	inline mat5_t<T> operator*(mat5_t<T> a) const {
		return mat5_t<T>(
			h.x1 * a.v.x1 + h.y1 * a.v.y1 + h.z1 * a.v.z1 + h.w1 * a.v.w1 + h.v1 * a.v.v1,
			h.x1 * a.v.x2 + h.y1 * a.v.y2 + h.z1 * a.v.z2 + h.w1 * a.v.w2 + h.v1 * a.v.v2,
			h.x1 * a.v.x3 + h.y1 * a.v.y3 + h.z1 * a.v.z3 + h.w1 * a.v.w3 + h.v1 * a.v.v3,
			h.x1 * a.v.x4 + h.y1 * a.v.y4 + h.z1 * a.v.z4 + h.w1 * a.v.w4 + h.v1 * a.v.v4,
			h.x1 * a.v.x5 + h.y1 * a.v.y5 + h.z1 * a.v.z5 + h.w1 * a.v.w5 + h.v1 * a.v.v5,
			h.x2 * a.v.x1 + h.y2 * a.v.y1 + h.z2 * a.v.z1 + h.w2 * a.v.w1 + h.v2 * a.v.v1,
			h.x2 * a.v.x2 + h.y2 * a.v.y2 + h.z2 * a.v.z2 + h.w2 * a.v.w2 + h.v2 * a.v.v2,
			h.x2 * a.v.x3 + h.y2 * a.v.y3 + h.z2 * a.v.z3 + h.w2 * a.v.w3 + h.v2 * a.v.v3,
			h.x2 * a.v.x4 + h.y2 * a.v.y4 + h.z2 * a.v.z4 + h.w2 * a.v.w4 + h.v2 * a.v.v4,
			h.x2 * a.v.x5 + h.y2 * a.v.y5 + h.z2 * a.v.z5 + h.w2 * a.v.w5 + h.v2 * a.v.v5,
			h.x3 * a.v.x1 + h.y3 * a.v.y1 + h.z3 * a.v.z1 + h.w3 * a.v.w1 + h.v3 * a.v.v1,
			h.x3 * a.v.x2 + h.y3 * a.v.y2 + h.z3 * a.v.z2 + h.w3 * a.v.w2 + h.v3 * a.v.v2,
			h.x3 * a.v.x3 + h.y3 * a.v.y3 + h.z3 * a.v.z3 + h.w3 * a.v.w3 + h.v3 * a.v.v3,
			h.x3 * a.v.x4 + h.y3 * a.v.y4 + h.z3 * a.v.z4 + h.w3 * a.v.w4 + h.v3 * a.v.v4,
			h.x3 * a.v.x5 + h.y3 * a.v.y5 + h.z3 * a.v.z5 + h.w3 * a.v.w5 + h.v3 * a.v.v5,
			h.x4 * a.v.x1 + h.y4 * a.v.y1 + h.z4 * a.v.z1 + h.w4 * a.v.w1 + h.v4 * a.v.v1,
			h.x4 * a.v.x2 + h.y4 * a.v.y2 + h.z4 * a.v.z2 + h.w4 * a.v.w2 + h.v4 * a.v.v2,
			h.x4 * a.v.x3 + h.y4 * a.v.y3 + h.z4 * a.v.z3 + h.w4 * a.v.w3 + h.v4 * a.v.v3,
			h.x4 * a.v.x4 + h.y4 * a.v.y4 + h.z4 * a.v.z4 + h.w4 * a.v.w4 + h.v4 * a.v.v4,
			h.x4 * a.v.x5 + h.y4 * a.v.y5 + h.z4 * a.v.z5 + h.w4 * a.v.w5 + h.v4 * a.v.v5,
			h.x5 * a.v.x1 + h.y5 * a.v.y1 + h.z5 * a.v.z1 + h.w5 * a.v.w1 + h.v5 * a.v.v1,
			h.x5 * a.v.x2 + h.y5 * a.v.y2 + h.z5 * a.v.z2 + h.w5 * a.v.w2 + h.v5 * a.v.v2,
			h.x5 * a.v.x3 + h.y5 * a.v.y3 + h.z5 * a.v.z3 + h.w5 * a.v.w3 + h.v5 * a.v.v3,
			h.x5 * a.v.x4 + h.y5 * a.v.y4 + h.z5 * a.v.z4 + h.w5 * a.v.w4 + h.v5 * a.v.v4,
			h.x5 * a.v.x5 + h.y5 * a.v.y5 + h.z5 * a.v.z5 + h.w5 * a.v.w5 + h.v5 * a.v.v5);
	}
	inline vec5_t<T> operator*(vec5_t<T> a) const {
		return vec5_t<T>(
			h.x1 * a.x + h.y1 * a.y + h.z1 * a.z + h.w1 * a.w + h.v1 * a.v,
			h.x2 * a.x + h.y2 * a.y + h.z2 * a.z + h.w2 * a.w + h.v2 * a.v,
			h.x3 * a.x + h.y3 * a.y + h.z3 * a.z + h.w3 * a.w + h.v3 * a.v,
			h.x4 * a.x + h.y4 * a.y + h.z4 * a.z + h.w4 * a.w + h.v4 * a.v,
			h.x5 * a.x + h.y5 * a.y + h.z5 * a.z + h.w5 * a.w + h.v5 * a.v);
	}
	inline mat5_t<T> operator*(T a) const {
		return mat5_t<T>(
			h.x1 * a, h.y1 * a, h.z1 * a, h.w1 * a, h.v1 * a,
			h.x2 * a, h.y2 * a, h.z2 * a, h.w2 * a, h.v2 * a,
			h.x3 * a, h.y3 * a, h.z3 * a, h.w3 * a, h.v3 * a,
			h.x4 * a, h.y4 * a, h.z4 * a, h.w4 * a, h.v4 * a,
			h.x5 * a, h.y5 * a, h.z5 * a, h.w5 * a, h.v5 * a);
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

} // namespace AzCore

template <typename T>
inline AzCore::vec5_t<T> operator*(AzCore::vec5_t<T> a, AzCore::mat5_t<T> b) {
	return AzCore::vec5_t<T>(
		a.x * b.v.x1 + a.y * b.v.y1 + a.z * b.v.z1 + a.w * b.v.w1 + a.v * b.v.v1,
		a.x * b.v.x2 + a.y * b.v.y2 + a.z * b.v.z2 + a.w * b.v.w2 + a.v * b.v.v2,
		a.x * b.v.x3 + a.y * b.v.y3 + a.z * b.v.z3 + a.w * b.v.w3 + a.v * b.v.v3,
		a.x * b.v.x4 + a.y * b.v.y4 + a.z * b.v.z4 + a.w * b.v.w4 + a.v * b.v.v4,
		a.x * b.v.x5 + a.y * b.v.y5 + a.z * b.v.z5 + a.w * b.v.w5 + a.v * b.v.v5);
}

#endif // AZCORE_MATH_MAT5_T_HPP
