#pragma once

#include "constants.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/epsilon.hpp>

namespace aln::Maths
{

/// @brief Absolute value of a
template <typename T>
inline T Abs(T a) { return glm::abs(a); }

template <typename T>
inline T Min(const T& a, const T& b) { return glm::min(a, b); }

template <typename T>
inline T Max(const T& a, const T& b) { return glm::max(a, b); }

template <typename T>
inline T Floor(const T& a) { return glm::floor(a); }

inline bool IsNearEqual(float a, float b, float eps = Epsilon) { return glm::epsilonEqual(a, b, eps); }
inline bool IsNearZero(float a, float eps = Epsilon) { return glm::epsilonEqual(a, 0.0f, eps); }
inline float Lerp(float a, float b, float t) { return a + t * (b - a); }

/// @brief Clamp the value of a between low and high
template <typename T>
inline T Clamp(const T& a, const T& low, const T& high) { return glm::clamp(a, low, high); }

/// @brief Returns the fractional part of a and sets i to the integer part
template <typename T>
inline T Modf(const T& a, T& integralPart) { return glm::modf(a, integralPart); }

template <typename T>
inline T Log2(const T& a) { return glm::log2(a); }

inline float SafeDivide(float a, float b, float eps = Maths::Epsilon) { return Abs(b) < eps ? 0.0f : a / b; }
} // namespace aln::Maths