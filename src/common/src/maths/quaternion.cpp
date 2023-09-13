#include "maths/quaternion.hpp"

#include "maths/matrix4x4.hpp"
#include "maths/vec3.hpp"

namespace aln
{
Quaternion Quaternion ::FromEulerAngles(const Vec3& eulerAnglesInRadians) { return Quaternion(glm::quat(eulerAnglesInRadians.AsGLM())); }

Vec3 Quaternion::AsEulerAngles() const
{
    return Vec3(glm::eulerAngles(AsGLM()));
}

Matrix4x4 Quaternion::AsMatrix() const { return Matrix4x4(glm::toMat4(AsGLM())); }

Vec3 Quaternion::RotateVector(const Vec3& vector) const { return glm::rotate(AsGLM(), vector.AsGLM()); }

const Quaternion Quaternion::Identity = Quaternion(1.0f, 0.0f, 0.0f, 0.0f);

} // namespace aln