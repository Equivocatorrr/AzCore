/*
	File: complex_t.hpp
	Author: Philip Haynes
*/

#ifndef AZCORE_MATH_COMPLEX_T_HPP
#define AZCORE_MATH_COMPLEX_T_HPP

#include "vec2_t.hpp"

#include "basic.hpp"

namespace AzCore {

template <typename T>
struct complex_t {
	union {
		struct {
			T real, imag;
		};
		struct {
			vec2_t<T> vector;
		};
		struct {
			T x, y;
		};
		struct {
			T data[2];
		};
	};

	complex_t() = default;
	inline complex_t(T a) : x(a), y(0) {}
	inline complex_t(T &a, T b) : x(a), y(b) {}
	inline complex_t(vec2_t<T> vec) : vector(vec) { }
	inline complex_t(T d[2]) : x(d[0]), y(d[1]) { }

	inline complex_t<T> operator*(complex_t<T> a) const {
		return complex_t<T>(real * a.real - imag * a.imag, real * a.imag + imag * a.real);
	}
	inline complex_t<T> operator*(T a) const {
		return complex_t<T>(real * a, imag * a);
	}
	inline complex_t<T> operator/(complex_t<T> a) const {
		return (*this) * a.Reciprocal();
	}
	inline complex_t<T> operator/(T a) const {
		return complex_t<T>(real / a, imag / a);
	}
	inline complex_t<T> operator+(complex_t<T> a) const {
		return complex_t<T>(real + a.real, imag + a.imag);
	}
	inline complex_t<T> operator+(T a) const {
		return complex_t<T>(real + a, imag);
	}
	inline complex_t<T> operator-(complex_t<T> a) const {
		return complex_t<T>(real - a.real, imag - a.imag);
	}
	inline complex_t<T> operator-(T a) const {
		return complex_t<T>(real - a, imag);
	}
	inline complex_t<T> operator-() const {
		return complex_t<T>(-real, -imag);
	}
	inline complex_t<T>& operator+=(complex_t<T> a) {
		real += a.real;
		imag += a.imag;
		return *this;
	}
	inline complex_t<T>& operator-=(complex_t<T> a) {
		real -= a.real;
		imag -= a.imag;
		return *this;
	}
	inline complex_t<T>& operator*=(complex_t<T> a) {
		*this = *this * a;
		return *this;
	}
	inline complex_t<T>& operator/=(complex_t<T> a) {
		*this = *this / a;
		return *this;
	}
	inline complex_t<T>& operator+=(T a) {
		real += a;
		return *this;
	}
	inline complex_t<T>& operator-=(T a) {
		real -= a;
		return *this;
	}
	inline complex_t<T>& operator*=(T a) {
		real *= a;
		imag *= a;
		return *this;
	}
	inline complex_t<T>& operator/=(T a) {
		real /= a;
		imag /= a;
		return *this;
	}
	inline complex_t<T> Conjugate() const {
		return complex_t<T>(real, -imag);
	}
	inline complex_t<T> Reciprocal() const {
		return Conjugate() / (x * x + y * y);
	}
};

} // namespace AzCore

template <typename T>
inline AzCore::complex_t<T> operator*(T &a, AzCore::complex_t<T> b) {
	return b * a;
}

template <typename T>
inline AzCore::complex_t<T> operator/(T &a, AzCore::complex_t<T> b) {
	return AzCore::complex_t<T>(a) / b;
}

template <typename T>
inline AzCore::complex_t<T> operator+(T &a, AzCore::complex_t<T> b) {
	return b + a;
}

template <typename T>
inline AzCore::complex_t<T> operator-(T &a, AzCore::complex_t<T> b) {
	return -b + a;
}

template <typename T>
inline T abs(AzCore::complex_t<T> a) {
	return sqrt(a.x * a.x + a.y * a.y);
}

template <typename T>
AzCore::complex_t<T> exp(AzCore::complex_t<T> a) {
	return AzCore::complex_t<T>(cos(a.imag), sin(a.imag)) * exp(a.real);
}

template <typename T>
AzCore::complex_t<T> log(AzCore::complex_t<T> a) {
	return AzCore::complex_t<T>(log(abs(a)), atan2(a.imag, a.real));
}

template <typename T>
AzCore::complex_t<T> pow(AzCore::complex_t<T> &a, AzCore::complex_t<T> e) {
	return exp(log(a) * e);
}

template <typename T>
AzCore::complex_t<T> pow(AzCore::complex_t<T> &a, T e) {
	return exp(log(a) * e);
}

#endif // AZCORE_MATH_COMPLEX_T_HPP
