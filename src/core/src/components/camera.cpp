#include "components/camera.hpp"

#include <common/transform.hpp>

namespace aln
{

glm::mat4 Camera::GetViewMatrix() const
{
    Transform t = GetWorldTransform();
    return glm::lookAt(t.GetTranslation(), t.GetTranslation() + forward, up);
}

glm::mat4 Camera::GetProjectionMatrix(float aspectRatio) const
{
    glm::mat4 matrix = glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
    matrix[1][1] *= -1;
    return matrix;
}

glm::mat4 Camera::GetViewProjectionMatrix(float aspectRatio) const
{
    return GetProjectionMatrix(aspectRatio) * GetViewMatrix();
}

ALN_REGISTER_IMPL_BEGIN(COMPONENTS, aln::Camera)
ALN_REFLECT_MEMBER(fov)
ALN_REFLECT_MEMBER(nearPlane)
ALN_REFLECT_MEMBER(farPlane)
ALN_REFLECT_MEMBER(m_backgroundColor)
ALN_REGISTER_IMPL_END()
} // namespace aln