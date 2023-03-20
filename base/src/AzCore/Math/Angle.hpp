/*
	File: Angle.hpp
	Author: Philip Haynes
*/

#ifndef AZCORE_ANGLE_HPP
#define AZCORE_ANGLE_HPP

#include "basic.hpp"

namespace AzCore {

template <typename T>
class Radians;
template <typename T>
class Angle;

/*  class: Degrees
	Author: Philip Haynes
	A discrete type that represents an angle in degrees.    */
template <typename T>
class Degrees {
	T _value;

public:
	Degrees() = default;
	Degrees(T a) : _value(a) {}
	Degrees(Radians<T> a);
	Degrees<T> &operator+=(Degrees<T> other) {
		_value += other._value;
		return *this;
	}
	Degrees<T> &operator-=(Degrees<T> other) {
		_value -= other._value;
		return *this;
	}
	Degrees<T> &operator*=(Degrees<T> other) {
		_value *= other._value;
		return *this;
	}
	Degrees<T> &operator/=(Degrees<T> other) {
		_value /= other._value;
		return *this;
	}
	Degrees<T> operator+(Degrees<T> other) const { return Degrees<T>(_value + other._value); }
	Degrees<T> operator-(Degrees<T> other) const { return Degrees<T>(_value - other._value); }
	Degrees<T> operator*(Degrees<T> other) const { return Degrees<T>(_value * other._value); }
	Degrees<T> operator/(Degrees<T> other) const { return Degrees<T>(_value / other._value); }
	bool operator==(Degrees<T> other) const { return _value == other._value; }
	bool operator!=(Degrees<T> other) const { return _value != other._value; }
	bool operator<=(Degrees<T> other) const { return _value <= other._value; }
	bool operator>=(Degrees<T> other) const { return _value >= other._value; }
	bool operator<(Degrees<T> other) const { return _value < other._value; }
	bool operator>(Degrees<T> other) const { return _value > other._value; }
	Degrees<T> operator-() const { return Degrees<T>(-_value); }
	T value() const { return _value; }
	T& value() { return _value; }
};

/*  class: Radians
	Author: Philip Haynes
	A discrete type that represents an angle in radians.    */
template <typename T>
class Radians {
	T _value;

public:
	Radians() = default;
	Radians(T a) : _value(a) {}
	Radians(Angle<T> a) : _value(a.value()) {}
	Radians(Degrees<T> a);
	Radians<T> &operator+=(Radians<T> other) {
		_value += other._value;
		return *this;
	}
	Radians<T> &operator-=(Radians<T> other) {
		_value -= other._value;
		return *this;
	}
	Radians<T> &operator*=(Radians<T> other) {
		_value *= other._value;
		return *this;
	}
	Radians<T> &operator/=(Radians<T> other) {
		_value /= other._value;
		return *this;
	}
	Radians<T> operator+(Radians<T> other) const { return Radians<T>(_value + other._value); }
	Radians<T> operator-(Radians<T> other) const { return Radians<T>(_value - other._value); }
	Radians<T> operator*(Radians<T> other) const { return Radians<T>(_value * other._value); }
	Radians<T> operator/(Radians<T> other) const { return Radians<T>(_value / other._value); }
	bool operator==(Radians<T> other) const { return _value == other._value; }
	bool operator!=(Radians<T> other) const { return _value != other._value; }
	bool operator<=(Radians<T> other) const { return _value <= other._value; }
	bool operator>=(Radians<T> other) const { return _value >= other._value; }
	bool operator<(Radians<T> other) const { return _value < other._value; }
	bool operator>(Radians<T> other) const { return _value > other._value; }
	Radians<T> operator-() const { return Radians<T>(-_value); }
	T value() const { return _value; }
	T& value() { return _value; }
};

/*  class: Angle
	Author: Philip Haynes
	A discrete type to represent all angles while regarding the circular nature of angles.   */
template <typename T>
class Angle {
	Radians<T> _value;

public:
	Angle() = default;
	Angle(const T &other) : _value(Radians<T>(other)) {}
	Angle(Degrees<T> other) : _value(Radians<T>(other)) {}
	Angle(Radians<T> other) : _value(wrap(other.value(), Tau<T>::value)) {}
	Angle<T> &operator+=(Radians<T> other) { return *this = _value + other; }
	Angle<T> operator+(Radians<T> other) const { return Angle<T>(_value + other); }
	Angle<T> operator-(Radians<T> other) const { return Angle<T>(_value - other); }
	Angle<T> operator-(T other) const { return Angle<T>(_value - other); }
	Radians<T> operator-(Angle<T> other) const;
	bool operator==(Angle<T> other) const { return _value == other._value; }
	bool operator!=(Angle<T> other) const { return _value != other._value; }
	T value() const { return _value.value(); }
	T& value() { return _value.value(); }
};

template<> Degrees<f32>::Degrees(Radians<f32> a);
template<> Radians<f32>::Radians(Degrees<f32> a);
typedef Degrees<f32> Degrees32;
typedef Radians<f32> Radians32;
typedef Angle<f32> Angle32;
// Finds the shortest distance from one angle to another.
Radians32 angleDiff(Angle32 from, Angle32 to);

template<> Degrees<f64>::Degrees(Radians<f64> a);
template<> Radians<f64>::Radians(Degrees<f64> a);
typedef Degrees<f64> Degrees64;
typedef Radians<f64> Radians64;
typedef Angle<f64> Angle64;
// Finds the shortest distance from one angle to another.
Radians64 angleDiff(Angle64 from, Angle64 to);

template <typename T>
Radians<T> Angle<T>::operator-(Angle<T> to) const {
	return angleDiff(*this, to);
}

template <typename T>
inline Radians<T> angleDir(Angle<T> from, Angle<T> to) {
	return sign(angleDiff(from, to));
}

template <typename T>
bool arcContains(Angle<T> arcStart, Angle<T> arcEnd, Angle<T> test) {
	T a = (arcEnd-arcStart).value();
	T b;
	if (a < 0.0f) {
		a = -a;
		b = (test - arcEnd).value();
	} else {
		b = (test - arcStart).value();
	}
	return b >= 0.0f && b <= a;
}

} // namespace AzCore

template <typename T>
inline T sin(AzCore::Radians<T> a) { return sin(a.value()); }
template <typename T>
inline T cos(AzCore::Radians<T> a) { return cos(a.value()); }
template <typename T>
inline T tan(AzCore::Radians<T> a) { return tan(a.value()); }

template <typename T>
inline T sin(AzCore::Angle<T> a) { return sin(a.value()); }
template <typename T>
inline T cos(AzCore::Angle<T> a) { return cos(a.value()); }
template <typename T>
inline T tan(AzCore::Angle<T> a) { return tan(a.value()); }

#endif // AZCORE_ANGLE_HPP
