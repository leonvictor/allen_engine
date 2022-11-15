#include "entity_systems/camera_controller.hpp "

#include <functional>
#include <input/input_service.hpp>

#include <GLFW/glfw3.h>

namespace aln
{

void EditorCameraController::Update(const UpdateContext& context)
{
    auto pInputService = context.GetService<input::InputService>();
    auto pMouse = pInputService->GetMouse();

    // Zoom
    auto scrollDelta = pMouse->GetScrollDelta();
    if (scrollDelta.y != 0.0f)
    {
        m_pCameraInstance->OffsetLocalTransformPosition(m_pCameraInstance->forward * scrollDelta.y);
    }

    // Translate
    if (pMouse->IsHeld(GLFW_MOUSE_BUTTON_MIDDLE))
    {
        auto delta = pMouse->GetDelta() * m_translationSensitivity;
        m_pCameraInstance->OffsetLocalTransformPosition((m_pCameraInstance->up * delta.y) - (m_pCameraInstance->right * delta.x));
    }

    // Rotate
    if (pMouse->IsHeld(GLFW_MOUSE_BUTTON_RIGHT))
    {
        auto delta = pMouse->GetDelta() * m_rotationSensitivity;
        m_pCameraInstance->OffsetLocalTransformRotation(glm::quat(glm::radians(glm::vec3(-delta.y, delta.x, 0.0f))));
    }

    m_pCameraInstance->forward = glm::vec3(0.0f, 0.0f, 1.0f) * m_pCameraInstance->GetWorldTransform().GetRotation();
    m_pCameraInstance->right = glm::normalize(glm::cross(m_pCameraInstance->forward, m_pCameraInstance->world_up));
    m_pCameraInstance->up = glm::normalize(glm::cross(m_pCameraInstance->right, m_pCameraInstance->forward));
}

void EditorCameraController::RegisterComponent(IComponent* pComponent)
{
    // Associate the controlled camera
    auto pCameraComponent = dynamic_cast<Camera*>(pComponent);

    if (pCameraComponent == nullptr)
        return;

    m_pCameraInstance = pCameraComponent;
}

void EditorCameraController::UnregisterComponent(IComponent* pComponent)
{
    if (pComponent == m_pCameraInstance)
    {
        m_pCameraInstance = nullptr;
    }
}

ALN_REGISTER_IMPL_BEGIN(SYSTEM, aln::EditorCameraController)
ALN_REFLECT_MEMBER(m_rotationSensitivity)
ALN_REFLECT_MEMBER(m_translationSensitivity)
ALN_REGISTER_IMPL_END()
} // namespace aln