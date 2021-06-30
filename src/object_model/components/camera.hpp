#pragma once

#include "../spatial_component.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stdexcept>

class Camera : public SpatialComponent
{
    friend class EditorCameraController;

    enum class Type
    {
        Game,      // In-game camera (todo)
        SceneView, // Renders the scene view in the editor
        Preview,   // Renders to previews (todo)
    };

    Type m_type = Type::SceneView;

  public:
    glm::vec3 forward;
    glm::vec3 up;
    glm::vec3 right;

    // TODO: This shouldn't be in the camera component.
    glm::vec3 world_up = glm::vec3(0.0f, 1.0f, 0.0f);

    // TODO
    float fov = 45.0f;
    float nearPlane = 0.1f;
    float farPlane = 300.f;

    glm::mat4 GetViewMatrix() const
    {
        Transform t = GetWorldTransform();
        return glm::lookAt(t.position, t.position + forward, up);
    }

    void SetLocalTransform(Transform transform) override
    {
        SpatialComponent::SetLocalTransform(transform);
        UpdateOrientation();
    }

    void Initialize() override {}
    void Shutdown() override {}
    bool Load() override { return true; }
    void Unload() override {}

  private:
    void UpdateOrientation()
    {
        Transform t = GetLocalTransform();
        forward.x = cos(glm::radians(t.rotation.x)) * cos(glm::radians(t.rotation.y));
        forward.y = sin(glm::radians(t.rotation.y));
        forward.z = sin(glm::radians(t.rotation.x)) * cos(glm::radians(t.rotation.y));
        forward = glm::normalize(forward);

        right = glm::normalize(glm::cross(forward, world_up));
        up = glm::normalize(glm::cross(right, forward));
    }
};