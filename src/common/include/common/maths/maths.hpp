#pragma once

#include "constants.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/epsilon.hpp>

namespace aln::Maths
{

/// @brief Absolute value of a
template <typename T>
inline T Abs(const T& a) { return glm::abs(a); }

template <typename T>
inline float Sign(const T& a) { return glm::sign(a); }

template <typename T>
inline T Min(const T& a, const T& b) { return glm::min(a, b); }

template <typename T>
inline T Max(const T& a, const T& b) { return glm::max(a, b); }

/// @brief Round to the nearest integer
template <typename T>
inline T Round(const T& a) { return glm::round(a); }

/// @brief Round up to the nearest integer
template <typename T>
inline T Ceil(const T& a) { return glm::ceil(a); }

/// @brief Round down to the nearest integer
template <typename T>
inline T Floor(const T& a) { return glm::floor(a); }

template <typename T>
inline T Pow(const T& a, const T& pow) { return glm::pow(a, pow); }

template <typename T>
inline T Sqrt(const T& a)
{
    assert(a >= 0.0);
    return glm::sqrt(a);
}

inline bool IsNearEqual(float a, float b, float eps = Epsilon) { return glm::epsilonEqual(a, b, eps); }
inline bool IsNearZero(float a, float eps = Epsilon) { return glm::epsilonEqual(a, 0.0f, eps); }

/// @brief Linear interpolation between a and b by a factor t
inline float Lerp(float a, float b, float t) { return a + t * (b - a); }

/// @brief Return the factor at which v stands in the [a, b] interval
inline float InverseLerp(float a, float b, float v) { return (v - a) / (b - a); }

/// @brief Remap a value v from the source interval to the target one
inline float Remap(float sourceIntervalMin, float sourceIntervalMax, float targetIntervalMin, float targetIntervalMax, float v)
{
    float t = InverseLerp(sourceIntervalMin, sourceIntervalMax, v);
    return Lerp(targetIntervalMin, targetIntervalMax, t);
}

/// @brief Clamp the value of a between low and high
template <typename T>
inline T Clamp(const T& a, const T& low, const T& high) { return glm::clamp(a, low, high); }

/// @brief Returns the fractional part of a and sets i to its integer part
template <typename T>
inline T Modf(const T& a, T& integralPart) { return glm::modf(a, integralPart); }

/// @brief Modulo. Returns the remainder of a/b
template <typename T>
inline T Mod(const T& a, const T& b) { return glm::mod(a, b); }

template <typename T>
inline T Log2(const T& a) { return glm::log2(a); }

inline float SafeDivide(float a, float b, float eps = Maths::Epsilon) { return Abs(b) < eps ? 0.0f : a / b; }
} // namespace aln::Maths