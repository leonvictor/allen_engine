#include "maths/vec4.hpp"

#include "maths/vec3.hpp"

namespace aln
{
Vec4::Vec4(const Vec3& xyz, float w) : x(xyz.x), y(xyz.y), z(xyz.z), w(w) {}

const Vec4 Vec4::Zeroes = Vec4(0.0f, 0.0f, 0.0f, 0.0f);
const Vec4 Vec4::Ones = Vec4(1.0f, 1.0f, 1.0f, 1.0f);
} // namespace aln