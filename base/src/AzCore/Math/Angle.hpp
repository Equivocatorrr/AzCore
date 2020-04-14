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
class Degrees
{
    T _value;

public:
    Degrees() = default;
    Degrees(T a) : _value(a) {}
    // Degrees(const Degrees<T> &a) : _value(a._value) {}
    Degrees(const Radians<T> &a)
    {
        if constexpr (std::is_same<T, f32>())
        {
            _value = a.value() / tau * 360.0f;
        }
        else
        {
            _value = a.value() / tau64 * 360.0;
        }
    }
    Degrees<T> &operator+=(const Degrees<T> &other)
    {
        _value += other._value;
        return *this;
    }
    Degrees<T> &operator-=(const Degrees<T> &other)
    {
        _value -= other._value;
        return *this;
    }
    Degrees<T> &operator*=(const Degrees<T> &other)
    {
        _value *= other._value;
        return *this;
    }
    Degrees<T> &operator/=(const Degrees<T> &other)
    {
        _value /= other._value;
        return *this;
    }
    Degrees<T> operator+(const Degrees<T> &other) const { return Degrees<T>(_value + other._value); }
    Degrees<T> operator-(const Degrees<T> &other) const { return Degrees<T>(_value - other._value); }
    Degrees<T> operator*(const Degrees<T> &other) const { return Degrees<T>(_value * other._value); }
    Degrees<T> operator/(const Degrees<T> &other) const { return Degrees<T>(_value / other._value); }
    bool operator==(const Degrees<T> &other) const { return _value == other._value; }
    bool operator!=(const Degrees<T> &other) const { return _value != other._value; }
    bool operator<=(const Degrees<T> &other) const { return _value <= other._value; }
    bool operator>=(const Degrees<T> &other) const { return _value >= other._value; }
    bool operator<(const Degrees<T> &other) const { return _value < other._value; }
    bool operator>(const Degrees<T> &other) const { return _value > other._value; }
    Degrees<T> operator-() const { return Degrees<T>(-_value); }
    const T value() const { return _value; }
};

/*  class: Radians
    Author: Philip Haynes
    A discrete type that represents an angle in radians.    */
template <typename T>
class Radians
{
    T _value;

public:
    Radians() = default;
    Radians(T a) : _value(a) {}
    // Radians(const Radians<T> &a) : _value(a._value) {}
    Radians(const Angle<T> &a) : _value(a.value()) {}
    Radians(const Degrees<T> &a)
    {
        if constexpr (std::is_same<T, f32>())
        {
            _value = a.value() * tau / 360.0f;
        }
        else
        {
            _value = a.value() * tau64 / 360.0;
        }
    }
    // Radians<T>& operator=(const Radians<T>& other) {
    //     _value = other._value;
    //     return *this;
    // }
    Radians<T> &operator+=(const Radians<T> &other)
    {
        _value += other._value;
        return *this;
    }
    Radians<T> &operator-=(const Radians<T> &other)
    {
        _value -= other._value;
        return *this;
    }
    Radians<T> &operator*=(const Radians<T> &other)
    {
        _value *= other._value;
        return *this;
    }
    Radians<T> &operator/=(const Radians<T> &other)
    {
        _value /= other._value;
        return *this;
    }
    Radians<T> operator+(const Radians<T> &other) const { return Radians<T>(_value + other._value); }
    Radians<T> operator-(const Radians<T> &other) const { return Radians<T>(_value - other._value); }
    Radians<T> operator*(const Radians<T> &other) const { return Radians<T>(_value * other._value); }
    Radians<T> operator/(const Radians<T> &other) const { return Radians<T>(_value / other._value); }
    bool operator==(const Radians<T> &other) const { return _value == other._value; }
    bool operator!=(const Radians<T> &other) const { return _value != other._value; }
    bool operator<=(const Radians<T> &other) const { return _value <= other._value; }
    bool operator>=(const Radians<T> &other) const { return _value >= other._value; }
    bool operator<(const Radians<T> &other) const { return _value < other._value; }
    bool operator>(const Radians<T> &other) const { return _value > other._value; }
    Radians<T> operator-() const { return Radians<T>(-_value); }
    const T value() const { return _value; }
};

/*  class: Angle
    Author: Philip Haynes
    A discrete type to represent all angles while regarding the circular nature of angles.   */
template <typename T>
class Angle
{
    Radians<T> _value;

public:
    Angle() = default;
    // Angle(const Angle<T> &other) : _value(other._value) {}
    Angle(const T &other) : _value(Radians<T>(other)) {}
    Angle(const Degrees<T> &other) : _value(Radians<T>(other)) {}
    Angle(const Radians<T> &other)
    {
        _value = other;
        if constexpr (std::is_same<T, f32>())
        {
            while (_value > tau)
            {
                _value -= tau;
            }
            while (_value < 0.0f)
            {
                _value += tau;
            }
        }
        else
        {
            while (_value > tau64)
            {
                _value -= tau64;
            }
            while (_value < 0.0)
            {
                _value += tau64;
            }
        }
    }
    Angle<T> &operator+=(const Radians<T> &other) { return *this = _value + other; }
    Angle<T> operator+(const Radians<T> &other) const { return Angle<T>(_value + other); }
    Radians<T> operator-(const Angle<T> &other) const;
    bool operator==(const Angle<T> &other) const { return _value == other._value; }
    bool operator!=(const Angle<T> &other) const { return _value != other._value; }
    const T value() const { return _value.value(); }
};

// Finds the shortest distance from one angle to another.
#ifdef AZCORE_MATH_F32
typedef Degrees<f32> Degrees32;
typedef Radians<f32> Radians32;
typedef Angle<f32> Angle32;
Radians32 angleDiff(Angle32 from, Angle32 to);
#endif
#ifdef AZCORE_MATH_F64
typedef Degrees<f64> Degrees64;
typedef Radians<f64> Radians64;
typedef Angle<f64> Angle64;
Radians64 angleDiff(Angle64 from, Angle64 to);
#endif

template <typename T>
Radians<T> Angle<T>::operator-(const Angle<T> &to) const
{
    return angleDiff(*this, to);
}

template <typename T>
inline Radians<T> angleDir(const Angle<T> &from, const Angle<T> &to)
{
    return sign(angleDiff(from, to));
}

} // namespace AzCore

template <typename T>
inline T sin(const AzCore::Radians<T> &a) { return sin(a.value()); }
template <typename T>
inline T cos(const AzCore::Radians<T> &a) { return cos(a.value()); }
template <typename T>
inline T tan(const AzCore::Radians<T> &a) { return tan(a.value()); }

template <typename T>
inline T sin(const AzCore::Angle<T> &a) { return sin(a.value()); }
template <typename T>
inline T cos(const AzCore::Angle<T> &a) { return cos(a.value()); }
template <typename T>
inline T tan(const AzCore::Angle<T> &a) { return tan(a.value()); }

#endif // AZCORE_ANGLE_HPP
