#pragma once

#include <functional>
#include <iostream>

#include "camera.cpp"

#include "input/callback_context.hpp"
#include "input/input_action.hpp"
#include "input/input_context.hpp"
#include "input/input_system.hpp"

class EditorCameraController
{
  public:
    EditorCameraController(Camera* camera)
    {
        // Associate the controlled camera
        m_pCameraInstance = camera;

        // Register control callbacks
        // TODO: Add another level of indirection so that multiple input can trigger the same action
        auto& moveAction = m_inputContext.AddAction(GLFW_MOUSE_BUTTON_MIDDLE);
        moveAction.SetInteraction(Interaction::Type::Hold);
        moveAction.SetCallback(std::bind(&EditorCameraController::Move, *this, std::placeholders::_1));

        auto& rotateAction = m_inputContext.AddAction(GLFW_MOUSE_BUTTON_RIGHT);
        rotateAction.SetInteraction(Interaction::Type::Hold);
        rotateAction.SetCallback(std::bind(&EditorCameraController::Rotate, *this, std::placeholders::_1));

        // TODO: Registration should happen in the Context constructor
        m_inputContext.Enable();
        Input::RegisterContext(&m_inputContext);
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
        m_pCameraInstance->transform.position += (m_pCameraInstance->up * delta.y) - (m_pCameraInstance->right * delta.x);
    }

    void Rotate(CallbackContext context)
    {
        auto delta = Input::Mouse.GetDelta() * m_rotationSensitivity;
        m_pCameraInstance->transform.rotation.x += delta.x;
        m_pCameraInstance->transform.rotation.y += delta.y;

        m_pCameraInstance->UpdateOrientation();
    }
};