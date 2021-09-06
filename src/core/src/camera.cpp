#include "camera.hpp"

#include <common/transform.hpp>

namespace aln
{

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
} // namespace aln

ALN_REGISTER_IMPL_BEGIN(COMPONENTS, aln::Camera)
ALN_REFLECT_MEMBER(fov)
ALN_REFLECT_MEMBER(nearPlane)
ALN_REFLECT_MEMBER(farPlane)
ALN_REGISTER_IMPL_END()