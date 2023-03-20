/*
	File: Angle.cpp
	Author: Philip Haynes
*/

#include "../math.hpp"

namespace AzCore {

Radians32 angleDiff(Angle32 from, Angle32 to) {
	Radians32 diff = Radians32(to) - Radians32(from);
	return wrap(diff.value() + pi, tau) - pi;
}

template<> Degrees<f32>::Degrees(Radians<f32> a) {
	_value = a.value() / tau * 360.0f;
}

template<> Radians<f32>::Radians(Degrees<f32> a) {
	_value = a.value() * tau / 360.0f;
}

Radians64 angleDiff(Angle64 from, Angle64 to) {
	Radians64 diff = Radians64(to) - Radians64(from);
	return wrap(diff.value() + pi64, tau64) - pi64;
}

template<> Degrees<f64>::Degrees(Radians<f64> a) {
	_value = a.value() / tau64 * 360.0;
}

template<> Radians<f64>::Radians(Degrees<f64> a) {
	_value = a.value() * tau64 / 360.0;
}

} // namespace AzCore
