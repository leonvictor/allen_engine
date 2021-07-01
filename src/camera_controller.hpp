#pragma once

#include <functional>
#include <iostream>

#include "object_model/components/camera.hpp"
#include "object_model/entity_system.hpp"

#include "input/callback_context.hpp"
#include "input/input_action.hpp"
#include "input/input_context.hpp"
#include "input/input_system.hpp"

class EditorCameraController : IEntitySystem
{
  public:
    void RegisterComponent(IComponent* pComponent)
    {
        // Associate the controlled camera
        auto pCameraComponent = dynamic_cast<Camera*>(pComponent);

        if (pCameraComponent == nullptr)
            return;

        m_pCameraInstance = pCameraComponent;

        // Register control callbacks
        // TODO: Add another level of indirection so that multiple input can trigger the same action
        auto& moveAction = m_inputContext.AddAction(GLFW_MOUSE_BUTTON_MIDDLE);
        moveAction.SetInteraction(Interaction::Type::Hold);
        moveAction.SetCallback(std::bind(&EditorCameraController::Move, *this, std::placeholders::_1));

        auto& rotateAction = m_inputContext.AddAction(GLFW_MOUSE_BUTTON_RIGHT);
        rotateAction.SetInteraction(Interaction::Type::Hold);
        rotateAction.SetCallback(std::bind(&EditorCameraController::Rotate, *this, std::placeholders::_1));

        auto& zoomAction = m_inputContext.AddAction(Input::Mouse.TEMPORARY_SCROLL_ID);
        zoomAction.SetInteraction(Interaction::Type::Hold);
        zoomAction.SetCallback(std::bind(&EditorCameraController::Zoom, *this, std::placeholders::_1));

        // TODO: Registration should happen in the Context constructor
        m_inputContext.Enable();
        Input::RegisterContext(&m_inputContext);
    }

    void UnregisterComponent(IComponent* pComponent)
    {
        if (pComponent == m_pCameraInstance)
        {
            m_pCameraInstance == nullptr;
            m_inputContext.Disable();
            // TODO: Unregister input context
        }
    }

  private:
    InputContext m_inputContext;

    float m_rotationSensitivity = 0.1f;
    float m_translationSensitivity = 0.006f;
    Camera* m_pCameraInstance;

    /// @brief Translate the camera position on the 2D plane parallel to the screen.
    void Move(CallbackContext context)
    {
        auto delta = Input::Mouse.GetDelta() * m_translationSensitivity;
        m_pCameraInstance->ModifyTransform()->position += (m_pCameraInstance->up * delta.y) - (m_pCameraInstance->right * delta.x);
    }

    void Rotate(CallbackContext context)
    {
        auto delta = Input::Mouse.GetDelta() * m_rotationSensitivity;

        auto t = m_pCameraInstance->ModifyTransform();
        t->rotation.x += delta.x;
        t->rotation.y += delta.y;
    }

    void Zoom(CallbackContext context)
    {
        auto delta = Input::Mouse.GetScroll();
        auto t = m_pCameraInstance->ModifyTransform()->position += m_pCameraInstance->forward * delta.y;
    }
};