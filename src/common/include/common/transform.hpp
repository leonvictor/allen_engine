#pragma once

#include <glm/gtx/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <aln_common_export.h>

namespace aln
{

/// @note ALN relies on glm, which uses column major order for matrices.
// The multiplication order is thus right-to-left (v' = T*v). The transform combination API follows the same semantics,
// i.e. ObjectWorldTransform = WorldTransform * ObjectLocalTransform
class ALN_COMMON_EXPORT Transform
{
  private:
    glm::vec3 m_translation = glm::vec3(0.0f);
    glm::quat m_rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec3 m_scale = glm::vec3(1.0f);

    /// @brief Helper function to check if a quaternion is normalized.
    /// @note Very ineficient, it's only use is for assertions which will be stripped in release.
    /// @todo Hide it better !
    bool IsQuaternionNormalized(const glm::quat& q)
    {
        auto n = glm::normalize(q);
        return glm::all(glm::epsilonEqual(q, n, Transform::Epsilon));
    };

  public:
    Transform() = default;
    Transform(const glm::mat4x4& matrix);
    Transform(const glm::vec3& translation, const glm::quat& rotation, const glm::vec3& scale)
        : m_translation(translation), m_rotation(rotation), m_scale(scale)
    {
        assert(IsQuaternionNormalized(rotation));
    }

    inline glm::vec3 GetTranslation() const { return m_translation; }
    inline glm::quat GetRotation() const { return m_rotation; }
    inline glm::vec3 GetScale() const { return m_scale; }
    inline glm::vec3 GetRotationEuler() const { return glm::degrees(glm::eulerAngles(m_rotation)); }

    inline void SetTranslation(const glm::vec3 translation) { m_translation = translation; }

    inline void SetRotation(const glm::quat rotation)
    {
        assert(IsQuaternionNormalized(rotation));
        m_rotation = rotation;
    }

    /// @brief Set the transform's rotation from euler angles given in degrees. Rotation order is XYZ.
    inline void SetRotationEuler(const glm::vec3 eulerAngles) { m_rotation = glm::normalize(glm::quat(glm::radians(eulerAngles))); }

    inline void SetScale(const glm::vec3 scale) { m_scale = scale; }

    glm::mat4x4 ToMatrix() const;

    Transform GetInverse() const;

    bool operator==(const Transform& b) const;
    bool operator!=(const Transform& b) const;

    /// @brief Transform composition (right-to-left)
    Transform& operator*=(const Transform& b);

    /// @brief Transform composition (right-to-left)
    Transform operator*(const Transform& b) const;

    glm::vec3 TransformPoint(const glm::vec3& point) const
    {
        glm::vec3 out = m_rotation * (m_scale * point);
        out = out + m_translation;
        return out;
    }

    glm::vec3 TransformVector(const glm::vec3& vector) const
    {
        glm::vec3 out = m_rotation * (m_scale * vector);
        return out;
    }

    static const Transform Identity;

    // TODO: Find a good place for epsilon
    static constexpr float Epsilon = 0.000001;
};

} // namespace aln
