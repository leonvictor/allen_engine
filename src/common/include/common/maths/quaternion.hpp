#pragma once

#include "constants.hpp"

#include <aln_common_export.h>

#include <glm/gtc/epsilon.hpp>
#include <glm/gtx/quaternion.hpp>

namespace aln
{

class Vec3;

class ALN_COMMON_EXPORT Quaternion
{
    friend class Matrix4x4;

  private:
    Quaternion(const glm::quat& quat) : w(quat.w), x(quat.x), y(quat.y), z(quat.z) {}
    glm::quat AsGLM() const { return glm::quat(w, x, y, z); }

  public:
    float w, x, y, z;

    Quaternion() = default;
    Quaternion(float w, float x, float y, float z) : w(w), x(x), y(y), z(z) {}

    /// @brief Construct a quaternion from euler angles (in radians)
    static Quaternion FromEulerAngles(const Vec3& eulerAnglesInRadians);

    inline Quaternion Normalized() const { return Quaternion(glm::normalize(AsGLM())); };
    inline Quaternion GetInverse() const { return Quaternion(glm::inverse(AsGLM())); };
    inline Quaternion GetConjugate() const { return Quaternion(glm::conjugate(AsGLM())); }

    Vec3 AsEulerAngles() const;
    Matrix4x4 AsMatrix() const;

    Vec3 RotateVector(const Vec3& vector) const;

    static Quaternion Slerp(const Quaternion& a, const Quaternion& b, float t) { return Quaternion(glm::slerp(a.AsGLM(), b.AsGLM(), t)); }

    inline bool IsNearEqual(const Quaternion& other, float eps = Maths::Epsilon) const { return glm::all(glm::epsilonEqual(AsGLM(), other.AsGLM(), eps)); }

    Quaternion operator*(const Quaternion& other) const { return Quaternion(AsGLM() * other.AsGLM()); }
    bool operator==(const Quaternion& other) const { return w == other.w && x == other.x && y == other.y && z == other.z; }
    bool operator!=(const Quaternion& other) const { return (*this == other); }

    static const Quaternion Identity;
};

} // namespace aln