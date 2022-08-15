#pragma once

#include <algorithm>
#include <cmath>

// todo: templatize ?
namespace aln::Math
{

static const float EPSILON = 0.0000001;

/// @brief Absolute value of a
template <typename T>
inline float Abs(T a) { return std::abs(a); }

template <typename T>
inline bool IsNearZero(T a, T eps = EPSILON) { return std::abs(a) < eps; }

template <typename T>
inline bool IsNearEqual(T a, T v, float eps = EPSILON)
{
    return glm::nearEqual(a, v, eps);
}

/// @brief Clamp the value of a between low and high
template <typename T>
inline float Clamp(T a, T low, T high)
{
    return std::clamp(a, low, high);
}

/// @brief Linear interpolation between a and b by a t factor
template <typename T>
inline float Lerp(T a, T b, T t) { return a + t * (b - a); }

} // namespace aln::Math