#include "entity_systems/camera_controller.hpp "

#include <input/devices/mouse.hpp>
#include <input/input_service.hpp>

#include <GLFW/glfw3.h>

#include <functional>

namespace aln
{

void EditorCameraController::Update(const UpdateContext& context)
{
    auto pInputService = context.GetService<InputService>();
    auto pMouse = pInputService->GetMouse();

    // Zoom
    auto scrollDelta = pMouse->GetScrollDelta();
    if (scrollDelta.y != 0.0f)
    {
        m_pCameraInstance->OffsetLocalTransformPosition(m_pCameraInstance->forward * scrollDelta.y);
    }

    // Translate
    if (pMouse->IsHeld(Mouse::Button::Middle))
    {
        auto delta = pMouse->GetDelta() * m_translationSensitivity;
        m_pCameraInstance->OffsetLocalTransformPosition((m_pCameraInstance->up * delta.y) - (m_pCameraInstance->right * delta.x));
    }

    // Rotate
    if (pMouse->IsHeld(Mouse::Button::Right))
    {
        auto delta = pMouse->GetDelta() * m_rotationSensitivity;
        m_pCameraInstance->OffsetLocalTransformRotation(Quaternion::FromEulerAngles(Vec3(-delta.y, delta.x, 0.0f).ToRadians()));
    }

    m_pCameraInstance->forward = m_pCameraInstance->GetWorldTransform().GetForwardVector();
    // TODO: Use GetRightVector and GetUpVector ?
    m_pCameraInstance->right = m_pCameraInstance->forward.Cross(m_pCameraInstance->world_up).Normalized();
    m_pCameraInstance->up = m_pCameraInstance->right.Cross(m_pCameraInstance->forward).Normalized();
}

void EditorCameraController::RegisterComponent(IComponent* pComponent)
{
    // Associate the controlled camera
    auto pCameraComponent = dynamic_cast<CameraComponent*>(pComponent);

    if (pCameraComponent == nullptr)
    {
        return;
    }

    m_pCameraInstance = pCameraComponent;
}

void EditorCameraController::UnregisterComponent(IComponent* pComponent)
{
    if (pComponent == m_pCameraInstance)
    {
        m_pCameraInstance = nullptr;
    }
}

ALN_REGISTER_IMPL_BEGIN(SYSTEMS, aln::EditorCameraController)
ALN_REFLECT_MEMBER(m_rotationSensitivity, Rotation Sensitivity)
ALN_REFLECT_MEMBER(m_translationSensitivity, Translation Sensitivity)
ALN_REGISTER_IMPL_END()
} // namespace aln