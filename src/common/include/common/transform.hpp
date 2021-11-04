#pragma once

#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/vec3.hpp>

namespace aln
{

// fwd
/// @todo I'd like to get rid of this type of cross-module forward declaration. It's only use is to give SpatialComponent the friend status to only allow it to modify a transform.
/// we could also extend Transform in the entities module but we would have to duplicate methods...
namespace entities
{
class SpatialComponent;
}

// TODO: rotation should probably be in quaternions or matrix format
// but we need to access it as euler angles. How do we handle this ?
// Simple getter/setter is not possible as we need to access values directly in imgui
class Transform
{
    friend class entities::SpatialComponent;

  private:
    glm::vec3 m_position = glm::vec3(0.0f);
    glm::quat m_rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec3 m_scale = glm::vec3(1.0f);

    glm::vec3 m_rotationEuler = glm::vec3(0.0f);

  public:
    glm::vec3 GetPosition() const { return m_position; }
    glm::quat GetRotation() const { return m_rotation; }
    glm::vec3 GetScale() const { return m_scale; }
    glm::vec3 GetRotationEuler() const { return m_rotationEuler; }

    Transform& operator=(const Transform& other);
    bool operator==(const Transform& b);
    bool operator!=(const Transform& b);
};

} // namespace aln
