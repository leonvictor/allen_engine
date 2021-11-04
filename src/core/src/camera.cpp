#include "camera.hpp"

#include <common/transform.hpp>

namespace aln
{

glm::mat4 Camera::GetViewMatrix() const
{
    Transform t = GetWorldTransform();
    return glm::lookAt(t.GetPosition(), t.GetPosition() + forward, up);
}

ALN_REGISTER_IMPL_BEGIN(COMPONENTS, aln::Camera)
ALN_REFLECT_MEMBER(fov)
ALN_REFLECT_MEMBER(nearPlane)
ALN_REFLECT_MEMBER(farPlane)
ALN_REFLECT_MEMBER(m_backgroundColor)
ALN_REGISTER_IMPL_END()
}