#pragma once

#include "angles.hpp"

namespace aln::Maths
{
inline float Cos(Radians a) { return glm::cos((float) a); }
inline float Sin(Radians a) { return glm::sin((float) a); }
inline float Tan(Radians a) { return glm::tan((float) a); }

inline Radians Acos(float a)
{
    assert(a >= -1.0f && a <= 1.0f);
    return (Radians) glm::acos(a);
}
inline Radians Asin(float a)
{
    assert(a >= -1.0f && a <= 1.0f);
    return (Radians) glm::asin(a);
}
inline Radians Atan(float a)
{
    assert(a >= -1.0f && a <= 1.0f);
    return (Radians) glm::atan(a);
}
} // namespace aln::Maths