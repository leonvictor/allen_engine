#pragma once

#include <glm/vec3.hpp>

namespace aln
{
struct Vec3 : public glm::vec3<float>
{
    inline void Normalize() { glm::normalize(*this); }
    inline float Magnitude() { return glm::length(*this); }
    inline float SquaredMagnitude() { return glm::length2(*this); }
};
} // namespace aln