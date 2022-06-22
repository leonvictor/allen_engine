#pragma once

#include <glm/gtx/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <aln_common_export.h>

namespace aln
{

class ALN_COMMON_EXPORT Transform
{
  private:
    glm::vec3 m_translation = glm::vec3(0.0f);
    glm::quat m_rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec3 m_scale = glm::vec3(1.0f);

  public:
    Transform() = default;
    Transform(const glm::mat4x4& matrix);

    inline glm::vec3 GetTranslation() const { return m_translation; }
    inline glm::quat GetRotation() const { return m_rotation; }
    inline glm::vec3 GetScale() const { return m_scale; }
    inline glm::vec3 GetRotationEuler() const { return glm::degrees(glm::eulerAngles(m_rotation)); }

    inline void SetTranslation(const glm::vec3 translation) { m_translation = translation; }
    inline void SetRotation(const glm::quat rotation) { m_rotation = rotation; }
    inline void SetRotationEuler(const glm::vec3 eulerAngles) { m_rotation = glm::quat(glm::radians(eulerAngles)); }
    inline void SetScale(const glm::vec3 scale) { m_scale = scale; }

    glm::mat4x4 ToMatrix() const;

    Transform GetInverse() const;

    Transform& operator=(const Transform& other);
    bool operator==(const Transform& b) const;
    bool operator!=(const Transform& b) const;

    void operator*=(const Transform& b);
    Transform operator*(const Transform& b) const;

    static const Transform Identity;

    static constexpr float Epsilon = 0.000001;
};

} // namespace aln
