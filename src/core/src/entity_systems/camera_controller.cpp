#include "entity_systems/camera_controller.hpp "

#include <input/devices/mouse.hpp>
#include <input/input_service.hpp>
#include <common/maths/quaternion.hpp>
#include <common/maths/angles.hpp>

#include <GLFW/glfw3.h>

#include <functional>

namespace aln
{

ALN_REGISTER_IMPL_BEGIN(SYSTEMS, EditorCameraController)
ALN_REFLECT_MEMBER(m_rotationSensitivity)
ALN_REFLECT_MEMBER(m_translationSensitivity)
ALN_REGISTER_IMPL_END()

void EditorCameraController::Update(const UpdateContext& context)
{
    auto pInputService = context.GetService<InputService>();
    auto pMouse = pInputService->GetMouse();

    // Zoom
    auto scrollDelta = pMouse->GetScrollDelta();
    if (scrollDelta.y != 0.0f)
    {
        m_pCameraInstance->OffsetLocalTransformPosition(m_pCameraInstance->GetCameraForwardVector() * scrollDelta.y);
    }

    // Translate
    if (pMouse->IsHeld(Mouse::Button::Middle))
    {
        auto delta = pMouse->GetDelta() * m_translationSensitivity;
        m_pCameraInstance->OffsetLocalTransformPosition((m_pCameraInstance->GetCameraUpVector() * delta.y) - (m_pCameraInstance->GetCameraRightVector() * delta.x));
    }

    // Rotate
    if (pMouse->IsHeld(Mouse::Button::Right))
    {
        auto delta = pMouse->GetDelta() * m_rotationSensitivity;
        
        auto yawAngle = Degrees(delta.x).ToRadians();
        auto yawRotation = Quaternion::FromAxisAngle(Vec3::Y, yawAngle);
        
        auto pitchAngle = Degrees(-delta.y).ToRadians();
        auto pitchRotation = Quaternion::FromAxisAngle(Vec3::X, pitchAngle);

        auto q = pitchRotation * m_pCameraInstance->GetLocalTransform().GetRotation() * yawRotation;
        m_pCameraInstance->SetLocalTransformRotation(q.Normalized());
    }
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

} // namespace aln