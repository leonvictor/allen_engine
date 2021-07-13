#include "camera.hpp"

glm::mat4 Camera::GetViewMatrix() const
{
    Transform t = GetWorldTransform();
    return glm::lookAt(t.position, t.position + forward, up);
}

void Camera::AfterTransformUpdate()
{
    Transform t = GetLocalTransform();
    forward.x = cos(glm::radians(t.rotation.x)) * cos(glm::radians(t.rotation.y));
    forward.y = sin(glm::radians(t.rotation.y));
    forward.z = sin(glm::radians(t.rotation.x)) * cos(glm::radians(t.rotation.y));
    forward = glm::normalize(forward);

    right = glm::normalize(glm::cross(forward, world_up));
    up = glm::normalize(glm::cross(right, forward));
}