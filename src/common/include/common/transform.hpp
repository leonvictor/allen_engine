#pragma once

#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/vec3.hpp>

namespace aln
{

class Transform
{
  private:
    glm::vec3 m_translation = glm::vec3(0.0f);
    glm::quat m_rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec3 m_scale = glm::vec3(1.0f);

  public:
    glm::vec3 GetTranslation() const { return m_translation; }
    glm::quat GetRotation() const { return m_rotation; }
    glm::vec3 GetScale() const { return m_scale; }
    glm::vec3 GetRotationEuler() const { return glm::degrees(glm::eulerAngles(m_rotation)); }

    void SetTranslation(const glm::vec3 translation) { m_translation = translation; }
    void SetRotation(const glm::quat rotation) { m_rotation = rotation; }
    void SetRotationEuler(const glm::vec3 eulerAngles) { m_rotation = glm::quat(glm::radians(eulerAngles)); }
    void SetScale(const glm::vec3 scale) { m_scale = scale; }

    Transform& operator=(const Transform& other);
    bool operator==(const Transform& b);
    bool operator!=(const Transform& b);
};

} // namespace aln
