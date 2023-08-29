#pragma once

#include <glm/gtx/compatibility.hpp>
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
    glm::vec3 m_translation;
    glm::quat m_rotation;
    glm::vec3 m_scale;

    /// @brief Helper function to check if a quaternion is normalized.
    /// @note Very ineficient, it's only use is for assertions which will be stripped in release.
    /// @todo Hide it better !
    bool IsQuaternionNormalized(const glm::quat& q)
    {
        auto n = glm::normalize(q);
        return glm::all(glm::epsilonEqual(q, n, Transform::Epsilon));
    };

  public:
    Transform() : m_translation(0.0f),
                  m_rotation(1.0f, 0.0f, 0.0f, 0.0f),
                  m_scale(1.0f){};
    Transform(const glm::mat4x4& matrix);
    Transform(const glm::vec3& translation, const glm::quat& rotation, const glm::vec3& scale)
        : m_translation(translation), m_rotation(rotation), m_scale(scale)
    {
        assert(IsQuaternionNormalized(rotation));
    }

    inline const glm::vec3& GetTranslation() const { return m_translation; }
    inline const glm::quat& GetRotation() const { return m_rotation; }
    inline const glm::vec3& GetScale() const { return m_scale; }
    inline const glm::vec3& GetRotationEuler() const { return glm::degrees(glm::eulerAngles(m_rotation)); }

    inline void SetTranslation(const glm::vec3& translation) { m_translation = translation; }
    inline void AddTranslation(const glm::vec3& translation) { m_translation = translation + m_translation; }
    
    inline void SetRotation(const glm::quat& rotation)
    {
        assert(IsQuaternionNormalized(rotation));
        m_rotation = rotation;
    }
    
    inline void AddRotation(const glm::quat& rotation) { m_rotation = rotation * m_rotation; }

    /// @brief Set the transform's rotation from euler angles given in degrees. Rotation order is XYZ.
    inline void SetRotationEuler(const glm::vec3& eulerAngles) { m_rotation = glm::normalize(glm::quat(glm::radians(eulerAngles))); }

    inline void SetScale(const glm::vec3& scale) { m_scale = scale; }

    glm::mat4x4 ToMatrix() const;

    Transform GetInverse() const;

    bool operator==(const Transform& b) const;
    bool operator!=(const Transform& b) const;

    static Transform Interpolate(const Transform& a, const Transform& b, float factor)
    {
        auto translation = glm::lerp(a.m_translation, b.m_translation, factor);
        auto rotation = glm::slerp(a.m_rotation, b.m_rotation, factor);
        auto scale = glm::lerp(a.m_scale, b.m_scale, factor);

        return Transform(translation, rotation, scale);
    };

    /// @brief Get the delta between two transforms. Scale is ignored !
    static Transform Delta(const Transform& from, const Transform& to)
    {
        const auto& inverseFromRotation = glm::inverse(from.m_rotation);
        
        Transform delta;
        delta.m_rotation = to.m_rotation * inverseFromRotation;
        delta.m_translation = glm::rotate(inverseFromRotation, to.m_translation - from.m_translation);

        return delta;
    }

    glm::vec3 TranslateVector(const glm::vec3& vector) const { return m_translation + vector; }
    glm::vec3 RotateVector(const glm::vec3& vector) const { return glm::rotate(m_rotation, vector); }
    glm::vec3 ScaleVector(const glm::vec3& vector) const { return m_scale * vector; }

    glm::vec3 TransformPoint(const glm::vec3& point) const
    {
        glm::vec3 out = m_rotation * (m_scale * point);
        out = out + m_translation;
        return out;
    }

    glm::vec3 TransformVector(const glm::vec3& vector) const
    {
        auto out = m_scale * vector;
        out = m_rotation * out;
        out = m_translation + out;
        return out;
    }

    /// @brief Transform composition (right-to-left)
    Transform& operator*=(const Transform& b);

    /// @brief Transform composition (right-to-left)
    Transform operator*(const Transform& b) const;

    static const Transform Identity;

    // TODO: Find a good place for epsilon
    static constexpr float Epsilon = 0.000001;
};

} // namespace aln
