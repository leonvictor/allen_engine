#include "entity_systems/camera_controller.hpp "

#include <functional>
#include <input/input_action.hpp>
#include <input/input_service.hpp>

#include <GLFW/glfw3.h>

namespace aln
{

void EditorCameraController::Update(const UpdateContext& context)
{
    auto pInputService = context.GetService<input::InputService>();
    auto pMouse = pInputService->GetMouse();

    auto scrollDelta = pMouse->GetScrollDelta();
    if (scrollDelta.y != 0.0f)
    {
        m_pCameraInstance->OffsetLocalTransformPosition(m_pCameraInstance->forward * scrollDelta.y);
    }

    if (pMouse->IsHeld(GLFW_MOUSE_BUTTON_MIDDLE))
    {
        auto delta = pMouse->GetDelta() * m_translationSensitivity;
        m_pCameraInstance->OffsetLocalTransformPosition((m_pCameraInstance->up * delta.y) - (m_pCameraInstance->right * delta.x));
    }

    if (pMouse->IsHeld(GLFW_MOUSE_BUTTON_RIGHT))
    {
        auto delta = pMouse->GetDelta() * m_rotationSensitivity;
        m_pCameraInstance->OffsetLocalTransformRotation(glm::quat(glm::radians(glm::vec3(-delta.y, delta.x, 0.0f))));
    }

    m_pCameraInstance->forward = glm::vec3(0.0f, 0.0f, 1.0f) * m_pCameraInstance->GetWorldTransform().GetRotation();
    m_pCameraInstance->right = glm::normalize(glm::cross(m_pCameraInstance->forward, m_pCameraInstance->world_up));
    m_pCameraInstance->up = glm::normalize(glm::cross(m_pCameraInstance->right, m_pCameraInstance->forward));

    m_hasChanged = false;
}

void EditorCameraController::RegisterComponent(IComponent* pComponent)
{
    // Associate the controlled camera
    auto pCameraComponent = dynamic_cast<Camera*>(pComponent);

    if (pCameraComponent == nullptr)
        return;

    m_pCameraInstance = pCameraComponent;

    // Register control callbacks
    // TODO: Add another level of indirection so that multiple input can trigger the same action
    // auto& moveAction = m_inputContext.AddAction(GLFW_MOUSE_BUTTON_MIDDLE);
    // moveAction.SetInteraction(Interaction::Type::Hold);
    // moveAction.SetCallback(std::bind(&EditorCameraController::Move, this, std::placeholders::_1));

    // auto& rotateAction = m_inputContext.AddAction(GLFW_MOUSE_BUTTON_RIGHT);
    // rotateAction.SetInteraction(Interaction::Type::Hold);
    // rotateAction.SetCallback(std::bind(&EditorCameraController::Rotate, this, std::placeholders::_1));

    // auto& zoomAction = m_inputContext.AddAction(Input::Mouse().TEMPORARY_SCROLL_ID);
    // zoomAction.SetInteraction(Interaction::Type::Hold);
    // zoomAction.SetCallback(std::bind(&EditorCameraController::Zoom, this, std::placeholders::_1));

    // TODO: Registration should happen in the Context constructor
    // m_inputContext.Enable();
    // Input::RegisterContext(&m_inputContext);
}

void EditorCameraController::UnregisterComponent(IComponent* pComponent)
{
    if (pComponent == m_pCameraInstance)
    {
        m_pCameraInstance = nullptr;
        m_inputContext.Disable();
        // Input::UnregisterContext(&m_inputContext);
    }
}

// void EditorCameraController::Move(CallbackContext context)
// {
//     // auto delta = Input::Mouse().GetDelta() * m_translationSensitivity;
//     // m_pCameraInstance->OffsetLocalTransformPosition((m_pCameraInstance->up * delta.y) - (m_pCameraInstance->right * delta.x));
// }

// void EditorCameraController::Rotate(CallbackContext context)
// {
//     // // auto delta = Input::Mouse().GetDelta() * m_rotationSensitivity;
//     // m_pCameraInstance->OffsetLocalTransformRotation(glm::quat(glm::radians(glm::vec3(-delta.y, delta.x, 0.0f))));
//     // m_hasChanged = true;
// }

// void EditorCameraController::Zoom(CallbackContext context)
// {
//     // auto delta = Input::Mouse().GetScroll();
//     // m_pCameraInstance->OffsetLocalTransformPosition(m_pCameraInstance->forward * delta.y);
// }

ALN_REGISTER_IMPL_BEGIN(SYSTEM, aln::EditorCameraController)
ALN_REFLECT_MEMBER(m_rotationSensitivity)
ALN_REFLECT_MEMBER(m_translationSensitivity)
ALN_REGISTER_IMPL_END()
} // namespace aln