#include "maths/quaternion.hpp"

#include "maths/angles.hpp"
#include "maths/matrix4x4.hpp"
#include "maths/vec3.hpp"

namespace aln
{
// note: glm uses x = pitch, y = yaw, r = roll
Quaternion Quaternion::FromEulerAngles(const EulerAnglesRadians& eulerAngles)
{
    auto vec = glm::vec3((float) eulerAngles.pitch, (float) eulerAngles.yaw, (float) eulerAngles.roll);
    return Quaternion(glm::quat(vec)).Normalized();
}

EulerAnglesRadians Quaternion::ToEulerAngles() const
{
    auto vec = glm::eulerAngles(AsGLM());
    return EulerAnglesRadians((Radians) vec.y, (Radians) vec.x, (Radians) vec.z);
}

Matrix4x4 Quaternion::ToMatrix() const { return Matrix4x4(glm::toMat4(AsGLM())); }

Vec3 Quaternion::RotateVector(const Vec3& vector) const { return glm::rotate(AsGLM(), vector.AsGLM()); }

const Quaternion Quaternion::Identity = Quaternion(1.0f, 0.0f, 0.0f, 0.0f);

} // namespace aln