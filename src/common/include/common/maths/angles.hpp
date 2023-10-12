#pragma once

#include "maths.hpp"

#include <aln_common_export.h>


namespace aln
{
struct Radians;

struct ALN_COMMON_EXPORT Degrees
{
    float m_value = 0.0f;

    Degrees() = default;
    Degrees(float degrees) : m_value(degrees) {}
    inline Degrees(const Radians& radians);

    inline float ToFloat() const { return m_value; }
    inline Radians ToRadians() const;

    explicit inline operator float() const { return m_value; }
    explicit inline operator Radians() const;

    Degrees operator-() const { return Degrees(-m_value); }

    Degrees operator+(const Degrees& other) const { return Degrees(m_value + other.m_value); }
    Degrees operator-(const Degrees& other) const { return Degrees(m_value - other.m_value); }
    Degrees operator*(const Degrees& other) const { return Degrees(m_value * other.m_value); }
    Degrees operator/(const Degrees& other) const { return Degrees(m_value / other.m_value); }

    Degrees operator+(float value) const { return Degrees(m_value + value); }
    Degrees operator-(float value) const { return Degrees(m_value - value); }
    Degrees operator*(float value) const { return Degrees(m_value * value); }
    Degrees operator/(float value) const { return Degrees(m_value / value); }

    Degrees& operator+=(const Degrees& other) { m_value += other.m_value; return *this; }
    Degrees& operator-=(const Degrees& other) { m_value -= other.m_value; return *this; }
    Degrees& operator*=(const Degrees& other) { m_value *= other.m_value; return *this; }
    Degrees& operator/=(const Degrees& other) { m_value /= other.m_value; return *this; }

    Degrees& operator+=(float value) { m_value += value; return *this; }
    Degrees& operator-=(float value) { m_value -= value; return *this; }
    Degrees& operator*=(float value) { m_value *= value; return *this; }
    Degrees& operator/=(float value) { m_value /= value; return *this; }

    bool operator<(const Degrees& other) const { return m_value < other.m_value; }
    bool operator<=(const Degrees& other) const { return m_value <= other.m_value; }
    bool operator>(const Degrees& other) const { return m_value > other.m_value; }
    bool operator>=(const Degrees& other) const { return m_value >= other.m_value; }

    bool operator<(float value) const { return m_value < value; }
    friend bool operator<(float value, const Degrees& degrees) { return value < degrees.m_value; } 
    bool operator<=(float value) const { return m_value <= value; }
    friend bool operator<=(float value, const Degrees& degrees) { return value <= degrees.m_value; } 
    bool operator>(float value) const { return m_value > value; }
    friend bool operator>(float value, const Degrees& degrees) { return value > degrees.m_value; } 
    bool operator>=(float value) const { return m_value >= value; }
    friend bool operator>=(float value, const Degrees& degrees) { return value >= degrees.m_value; } 

    bool operator==(const Degrees& other) const { return Maths::IsNearEqual(m_value, other.m_value); }
    bool operator!=(const Degrees& other) const { return !Maths::IsNearEqual(m_value, other.m_value); }
    
    bool operator==(float value) const { return Maths::IsNearEqual(m_value, value); }
    bool operator!=(float value) const { return !Maths::IsNearEqual(m_value, value); }

    static Degrees Clamp(const Degrees& degrees, const Degrees& min, const Degrees& max) { return Degrees(Maths::Clamp(degrees.m_value, min.m_value, max.m_value)); }
    Degrees Clamped(const Degrees& min, const Degrees& max) const { return Clamp(*this, min, max); }

    /// @brief Wrap the angle to [0, 360]
    static Degrees Wrap360(const Degrees& degrees) { return Degrees(Maths::Clamp(degrees.m_value - Maths::Floor(degrees.m_value / 360.0f) * 360.0f, 0.0f, 360.0f)); }
    Degrees Wrapped360() const { return Wrap360(*this); }

    /// @brief Shortest difference from a to b
    static Degrees DeltaAngle(const Degrees& a, const Degrees& b)
    {
        auto delta = Degrees::Wrap360(b - a);
        return delta > 180.0f ? delta - 360.0f : delta;
    }
};

struct ALN_COMMON_EXPORT Radians
{
    float m_value = 0.0f;

    Radians() = default;
    inline Radians(float valueInRadians) : m_value(valueInRadians) {}
    inline Radians(const Degrees& degrees);

    inline float ToFloat() const { return m_value; }
    inline Degrees ToDegrees() const;

    explicit inline operator float() const { return m_value; }
    explicit inline operator Degrees() const { return ToDegrees(); };

    Radians operator-() const { return Radians(-m_value); }

    Radians operator+(const Radians& other) const { return Radians(m_value + other.m_value); }
    Radians operator-(const Radians& other) const { return Radians(m_value - other.m_value); }
    Radians operator*(const Radians& other) const { return Radians(m_value * other.m_value); }
    Radians operator/(const Radians& other) const { return Radians(m_value / other.m_value); }

    Radians operator+(float value) const { return Radians(m_value + value); }
    Radians operator-(float value) const { return Radians(m_value - value); }
    Radians operator*(float value) const { return Radians(m_value * value); }
    Radians operator/(float value) const { return Radians(m_value / value); }

    Radians& operator+=(const Radians& other) { m_value += other.m_value; return *this; }
    Radians& operator-=(const Radians& other) { m_value -= other.m_value; return *this; }
    Radians& operator*=(const Radians& other) { m_value *= other.m_value; return *this; }
    Radians& operator/=(const Radians& other) { m_value /= other.m_value; return *this; }

    Radians& operator+=(float value) { m_value += value; return *this; }
    Radians& operator-=(float value) { m_value -= value; return *this; }
    Radians& operator*=(float value) { m_value *= value; return *this; }
    Radians& operator/=(float value) { m_value /= value; return *this; }

    bool operator<(const Radians& other) const { return m_value < other.m_value; }
    bool operator<=(const Radians& other) const { return m_value <= other.m_value; }
    bool operator>(const Radians& other) const { return m_value > other.m_value; }
    bool operator>=(const Radians& other) const { return m_value >= other.m_value; }

    bool operator<(float value) const { return m_value < value; }
    friend bool operator<(float value, const Radians& Radians) { return value < Radians.m_value; } 
    bool operator<=(float value) const { return m_value <= value; }
    friend bool operator<=(float value, const Radians& Radians) { return value <= Radians.m_value; } 
    bool operator>(float value) const { return m_value > value; }
    friend bool operator>(float value, const Radians& Radians) { return value > Radians.m_value; } 
    bool operator>=(float value) const { return m_value >= value; }
    friend bool operator>=(float value, const Radians& Radians) { return value >= Radians.m_value; } 

    bool operator==(const Radians& other) const { return Maths::IsNearEqual(m_value, other.m_value); }
    bool operator!=(const Radians& other) const { return !Maths::IsNearEqual(m_value, other.m_value); }
    
    bool operator==(float value) const { return Maths::IsNearEqual(m_value, value); }
    bool operator!=(float value) const { return !Maths::IsNearEqual(m_value, value); }
};

inline Degrees::operator Radians() const { return ToRadians(); }
inline Degrees::Degrees(const Radians& radians) : m_value(glm::degrees(radians.ToFloat())) {}
inline Degrees Radians::ToDegrees() const { return Degrees(glm::degrees(m_value)); }

inline Radians::Radians(const Degrees& degrees) : m_value(glm::radians(degrees.ToFloat())) {}
inline Radians Degrees::ToRadians() const { return Radians(glm::radians(m_value)); }

// ---- Euler angles
struct EulerAnglesDegrees;

struct ALN_COMMON_EXPORT EulerAnglesRadians
{
    Radians yaw = 0.0f;
    Radians pitch = 0.0f;
    Radians roll = 0.0f;

    EulerAnglesRadians() = default;
    EulerAnglesRadians(const Radians& yaw, const Radians& pitch, const Radians& roll) : yaw(yaw), pitch(pitch), roll(roll) {}

    inline EulerAnglesDegrees ToDegrees() const;
};

struct ALN_COMMON_EXPORT EulerAnglesDegrees
{
    Degrees yaw = 0.0f;
    Degrees pitch = 0.0f;
    Degrees roll = 0.0f;

    EulerAnglesDegrees() = default;
    EulerAnglesDegrees(const Degrees& yaw, const Degrees& pitch, const Degrees& roll) : yaw(yaw), pitch(pitch), roll(roll) {}

    inline EulerAnglesRadians ToRadians() const;
};

inline EulerAnglesDegrees EulerAnglesRadians::ToDegrees() const { return EulerAnglesDegrees(yaw.ToDegrees(), pitch.ToDegrees(), roll.ToDegrees()); }
inline EulerAnglesRadians EulerAnglesDegrees::ToRadians() const { return EulerAnglesRadians(yaw.ToRadians(), pitch.ToRadians(), roll.ToRadians()); }

} // namespace aln