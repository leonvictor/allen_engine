#include "components/camera.hpp"

#include <common/transform.hpp>

namespace aln
{

ALN_REGISTER_IMPL_BEGIN(COMPONENTS, CameraComponent)
ALN_REFLECT_BASE(SpatialComponent)
ALN_REFLECT_MEMBER(m_fieldOfView)
ALN_REFLECT_MEMBER(m_nearPlane)
ALN_REFLECT_MEMBER(m_farPlane)
ALN_REFLECT_MEMBER(m_backgroundColor)
ALN_REGISTER_IMPL_END()

Matrix4x4 CameraComponent::GetViewMatrix() const
{
    Transform t = GetWorldTransform();
    return Matrix4x4::LookAt(t.GetTranslation(), t.GetTranslation() + forward, up);
}

Matrix4x4 CameraComponent::GetProjectionMatrix(float aspectRatio) const
{
    auto matrix = Matrix4x4::Perspective(m_fieldOfView, aspectRatio, m_nearPlane, m_farPlane);
    matrix[1][1] *= -1; // TODO: Move that to the construction fn ?
    return matrix;
}

Matrix4x4 CameraComponent::GetViewProjectionMatrix(float aspectRatio) const
{
    return GetProjectionMatrix(aspectRatio) * GetViewMatrix();
}
} // namespace aln