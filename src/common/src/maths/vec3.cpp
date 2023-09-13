#include "maths/vec3.hpp"

#include "maths/matrix4x4.hpp"

#include <glm/gtx/transform.hpp>

namespace aln
{

Matrix4x4 Vec3::AsTranslationMatrix() const { return Matrix4x4(glm::translate(AsGLM())); }
Matrix4x4 Vec3::AsScalingMatrix() const { return Matrix4x4(glm::scale(AsGLM())); }

const Vec3 Vec3::Zeroes = Vec3(0.0f, 0.0f, 0.f);
const Vec3 Vec3::Ones = Vec3(1.0f, 1.0f, 1.0f);
const Vec3 Vec3::X = Vec3(1.0f, 0.0f, 0.0f);
const Vec3 Vec3::Y = Vec3(0.0f, 1.0f, 0.0f);
const Vec3 Vec3::Z = Vec3(0.0f, 0.0f, 1.0f);
const Vec3 Vec3::WorldUp = Vec3(0.0f, 1.0f, 0.0f);
const Vec3 Vec3::WorldDown = Vec3(0.0f, -1.0f, 0.0f);
const Vec3 Vec3::WorldRight = Vec3(1.0f, 0.0f, 0.0f);
const Vec3 Vec3::WorldLeft = Vec3(-1.0f, 0.0f, 0.0f);
const Vec3 Vec3::WorldForward = Vec3(0.0f, 0.0f, 1.0f);
const Vec3 Vec3::WorldBackward = Vec3(0.0f, 0.0f, -1.0f);

} // namespace aln