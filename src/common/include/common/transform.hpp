#pragma once

#include "maths/maths.hpp"
#include "maths/quaternion.hpp"
#include "maths/vec3.hpp"

#include <aln_common_export.h>

namespace aln
{

class Matrix4x4;

/// @note ALN relies on glm, which uses column major order for matrices.
// The multiplication order is thus right-to-left (v' = T*v). The transform combination API follows the same semantics,
// i.e. ObjectWorldTransform = WorldTransform * ObjectLocalTransform
class ALN_COMMON_EXPORT Transform
{
  private:
    Vec3 m_translation;
    Quaternion m_rotation;
    Vec3 m_scale;

    /// @brief Helper function to check if a quaternion is normalized.
    /// @note Very ineficient, it's only use is for assertions which will be stripped in release.
    bool IsQuaternionNormalized(const Quaternion& q)
    {
        auto n = q.Normalized();
        return q.IsNearEqual(n);
    };

  public:
    Transform() : m_translation(Vec3::Zeroes),
                  m_rotation(Quaternion::Identity),
                  m_scale(Vec3::Ones){};
    Transform(const Matrix4x4& matrix);
    Transform(const Vec3& translation, const Quaternion& rotation, const Vec3& scale)
        : m_translation(translation), m_rotation(rotation), m_scale(scale)
    {
        assert(IsQuaternionNormalized(rotation));
    }

    inline const Vec3& GetTranslation() const { return m_translation; }
    inline const Quaternion& GetRotation() const { return m_rotation; }
    inline const Vec3& GetScale() const { return m_scale; }
    inline const Vec3 GetRotationEuler() const { return m_rotation.AsEulerAngles().ToDegrees(); }

    inline void SetTranslation(const Vec3& translation) { m_translation = translation; }
    inline void AddTranslation(const Vec3& translation) { m_translation = translation + m_translation; }

    inline void SetRotation(const Quaternion& rotation)
    {
        assert(IsQuaternionNormalized(rotation));
        m_rotation = rotation;
    }

    inline void AddRotation(const Quaternion& rotation) { m_rotation = rotation * m_rotation; }

    /// @brief Set the transform's rotation from euler angles given in degrees. Rotation order is XYZ.
    inline void SetRotationEuler(const Vec3& eulerAnglesInDegrees) { m_rotation = Quaternion::FromEulerAngles(eulerAnglesInDegrees.ToRadians()).Normalized(); }

    inline void SetScale(const Vec3& scale) { m_scale = scale; }

    Matrix4x4 ToMatrix() const;

    Transform GetInverse() const;

    bool operator==(const Transform& b) const;
    bool operator!=(const Transform& b) const;

    static Transform Interpolate(const Transform& a, const Transform& b, float factor)
    {
        auto translation = Vec3::Lerp(a.m_translation, b.m_translation, factor);
        auto rotation = Quaternion::Slerp(a.m_rotation, b.m_rotation, factor);
        auto scale = Vec3::Lerp(a.m_scale, b.m_scale, factor);

        return Transform(translation, rotation, scale);
    };

    /// @brief Get the delta between two transforms. Scale is ignored !
    static Transform Delta(const Transform& from, const Transform& to)
    {
        const auto& inverseFromRotation = from.m_rotation.GetInverse();

        Transform delta;
        delta.m_rotation = to.m_rotation * inverseFromRotation;
        delta.m_translation = inverseFromRotation.RotateVector(to.m_translation - from.m_translation);

        return delta;
    }

    Vec3 TranslateVector(const Vec3& vector) const { return m_translation + vector; }
    Vec3 RotateVector(const Vec3& vector) const { return m_rotation.RotateVector(vector); }
    Vec3 ScaleVector(const Vec3& vector) const { return m_scale * vector; }

    Vec3 TransformPoint(const Vec3& point) const
    {
        Vec3 out = m_rotation.RotateVector(m_scale * point);
        out = out + m_translation;
        return out;
    }

    Vec3 TransformVector(const Vec3& vector) const
    {
        auto out = m_scale * vector;
        out = m_rotation.RotateVector(out);
        out = m_translation + out;
        return out;
    }

    Vec3 GetAxisX() const { return m_rotation.RotateVector(Vec3::X); }
    Vec3 GetAxisY() const { return m_rotation.RotateVector(Vec3::Y); }
    Vec3 GetAxisZ() const { return m_rotation.RotateVector(Vec3::Z); }

    Vec3 GetUpVector()const  { return m_rotation.RotateVector(Vec3::WorldUp); }
    Vec3 GetForwardVector()const  { return m_rotation.RotateVector(Vec3::WorldForward); }
    Vec3 GetRightVector() const { return m_rotation.RotateVector(Vec3::WorldRight); }

    /// @brief Transform composition (right-to-left)
    Transform& operator*=(const Transform& b);

    /// @brief Transform composition (right-to-left)
    Transform operator*(const Transform& b) const;

    static const Transform Identity;
};

} // namespace aln
