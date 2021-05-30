#pragma once

#include "components.hpp"
#include "transform.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stdexcept>

// TODO: Camera is a SceneObject with a Transform and a Camera component
// TODO: Then the Camera component needs to access the Transform in order to move around
class Camera : public Component
{
    friend class EditorCameraController;

  public:
    Transform transform;
    glm::vec3 forward;

    glm::vec3 up;
    glm::vec3 right;
    glm::vec3 world_up = glm::vec3(0.0f, 1.0f, 0.0f); // TODO: Grab this from a program-wide const

    // TODO
    // float fov;

    Camera(glm::vec3 position = glm::vec3(0.0f))
    {
        transform.position = position;
        transform.rotation.x = 90.0f;
        UpdateOrientation();
    }

    void zoomForward(float offset)
    {
        transform.position += forward * offset;
    }

    void zoomBackward(float offset)
    {
        zoomForward(-offset);
    }

    void orbit()
    {
        // TODO
        throw std::runtime_error("Not implemented");
    }

    glm::mat4 getViewMatrix() const
    {
        return glm::lookAt(transform.position, transform.position + forward, up);
    }

  private:
    void UpdateOrientation()
    {
        forward.x = cos(glm::radians(transform.rotation.x)) * cos(glm::radians(transform.rotation.y));
        forward.y = sin(glm::radians(transform.rotation.y));
        forward.z = sin(glm::radians(transform.rotation.x)) * cos(glm::radians(transform.rotation.y));
        forward = glm::normalize(forward);

        right = glm::normalize(glm::cross(forward, world_up));
        up = glm::normalize(glm::cross(right, forward));
    }
};