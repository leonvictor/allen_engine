#include "entity_systems/camera_controller_system.hpp"

#include <input/devices/mouse.hpp>
#include <input/input_service.hpp>
#include <common/maths/quaternion.hpp>
#include <common/maths/angles.hpp>

#include <GLFW/glfw3.h>

#include <functional>

namespace aln
{

ALN_REGISTER_IMPL_BEGIN(SYSTEMS, EditorCameraControllerSystem)
ALN_REFLECT_MEMBER(m_rotationSensitivity)
ALN_REFLECT_MEMBER(m_translationSensitivity)
ALN_REGISTER_IMPL_END()

void EditorCameraControllerSystem::Update(const UpdateContext& context)
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
        auto degrees = EulerAnglesDegrees(delta.x, -delta.y, 0.0f);
        auto radians = degrees.ToRadians();
        auto quat = Quaternion::FromEulerAngles(radians);
        m_pCameraInstance->OffsetLocalTransformRotation(quat);
    }
}

void EditorCameraControllerSystem::RegisterComponent(IComponent* pComponent)
{
    // Associate the controlled camera
    auto pCameraComponent = dynamic_cast<CameraComponent*>(pComponent);

    if (pCameraComponent == nullptr)
    {
        return;
    }

    m_pCameraInstance = pCameraComponent;
}

void EditorCameraControllerSystem::UnregisterComponent(IComponent* pComponent)
{
    if (pComponent == m_pCameraInstance)
    {
        m_pCameraInstance = nullptr;
    }
}

} // namespace aln