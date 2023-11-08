#pragma once

#include "angles.hpp"
#include "constants.hpp"
#include "vec3.hpp"

#include <aln_common_export.h>

#include <glm/gtc/epsilon.hpp>
#include <glm/gtx/quaternion.hpp>

namespace aln
{

class EulerAnglesRadians;
class Vec3;

class ALN_COMMON_EXPORT Quaternion
{
    friend class Matrix4x4;

  private:
    Quaternion(const glm::quat& quat) : w(quat.w), x(quat.x), y(quat.y), z(quat.z) {}
    glm::quat AsGLM() const { return glm::quat(w, x, y, z); }

    bool IsNormalized() const { return *this == Normalized(); }

  public:
    float w, x, y, z;

    Quaternion() = default;
    Quaternion(float w, float x, float y, float z) : w(w), x(x), y(y), z(z) {}

    /// @brief Construct a quaternion from euler angles (in radians)
    static Quaternion FromEulerAngles(const EulerAnglesRadians& eulerAngles);
    static Quaternion FromAxisAngle(const Vec3& axis, const Radians& angle);
    static Quaternion FromRotationBetweenVectors(const Vec3& from, const Vec3& to);
    static Quaternion LookAt(const Vec3& forward, const Vec3& up = Vec3::WorldUp);

    EulerAnglesRadians ToEulerAngles() const;
    Matrix4x4 ToMatrix() const;

    inline bool IsNearEqual(const Quaternion& other, float eps = Maths::Epsilon) const { return glm::all(glm::epsilonEqual(AsGLM(), other.AsGLM(), eps)); }
    bool operator==(const Quaternion& other) const { return IsNearEqual(other); }
    bool operator!=(const Quaternion& other) const { return !IsNearEqual(other); }

    Quaternion operator*(const Quaternion& other) const
    {
        assert(IsNormalized() && other.IsNormalized());
        return Quaternion(AsGLM() * other.AsGLM());
    }

    inline Quaternion Inversed() const { return Quaternion(glm::inverse(AsGLM())); };
    inline Quaternion Conjugated() const { return Quaternion(glm::conjugate(AsGLM())); }
    inline Quaternion Normalized() const { return Quaternion(glm::normalize(AsGLM())); };

    Vec3 RotateVector(const Vec3& vector) const;

    static Quaternion Slerp(const Quaternion& a, const Quaternion& b, float t) { return Quaternion(glm::slerp(a.AsGLM(), b.AsGLM(), t)); }

    static const Quaternion Identity;
};

} // namespace aln