#include "camera.hpp"

#include <common/transform.hpp>

namespace aln
{

glm::mat4 Camera::GetViewMatrix() const
{
    Transform t = GetWorldTransform();
    return glm::lookAt(t.position, t.position + forward, up);
}

glm::mat4 Camera::GetProjectionMatrix(float aspectRatio) const
{
    auto projectionMatrix = glm::perspective(
        glm::radians(fov),
        aspectRatio,
        nearPlane,
        farPlane);

    projectionMatrix[1][1] *= -1; // GLM is designed for OpenGL which uses inverted y coordinates
    return projectionMatrix;
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
} // namespace aln