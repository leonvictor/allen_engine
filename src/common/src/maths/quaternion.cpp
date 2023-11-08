#include "maths/quaternion.hpp"

#include "maths/angles.hpp"
#include "maths/matrix4x4.hpp"
#include "maths/vec3.hpp"

namespace aln
{
/// @note : Rotation order is x (pitch) -> y (yaw) -> z (roll)
Quaternion Quaternion::FromEulerAngles(const EulerAnglesRadians& eulerAngles)
{
    auto x = Quaternion::FromAxisAngle(Vec3::X, eulerAngles.pitch);
    auto y = Quaternion::FromAxisAngle(Vec3::Y, eulerAngles.yaw);
    auto z = Quaternion::FromAxisAngle(Vec3::Z, eulerAngles.roll);

    return x * y * z;
}

Quaternion Quaternion::FromAxisAngle(const Vec3& axis, const Radians& angle)
{
    assert(axis.IsNormalized());
    return Quaternion(glm::angleAxis((float) angle, axis.AsGLM()));
}

Quaternion Quaternion::FromRotationBetweenVectors(const Vec3& from, const Vec3& to)
{
    assert(from.IsNormalized());
    assert(to.IsNormalized());

    auto rotationAxis = from.Cross(to).Normalized();
    assert(rotationAxis.IsNearZero());

    const auto dot = from.Dot(to);
    if (dot >= (1.0f - Maths::Epsilon))
    {
        return Quaternion::Identity;
    }
    else
    {
        auto angle = Maths::Acos(dot);
        return Quaternion::FromAxisAngle(rotationAxis, angle);
    }
}

Quaternion Quaternion::LookAt(const Vec3& forward, const Vec3& up)
{
    assert(forward.IsNormalized());
    assert(up.IsNormalized());
    return Quaternion(glm::quatLookAt(forward.AsGLM(), up.AsGLM()));
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