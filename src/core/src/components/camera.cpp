#include "components/camera.hpp"

#include <common/transform.hpp>

namespace aln
{

ALN_REGISTER_IMPL_BEGIN(COMPONENTS, Camera)
ALN_REFLECT_MEMBER(fov, Field of View)
ALN_REFLECT_MEMBER(nearPlane, Near Plane)
ALN_REFLECT_MEMBER(farPlane, Far Plane)
ALN_REFLECT_MEMBER(m_backgroundColor, Background Color)
ALN_REGISTER_IMPL_END()

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
} // namespace aln