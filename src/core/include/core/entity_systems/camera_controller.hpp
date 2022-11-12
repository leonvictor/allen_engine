#pragma once

#include "../components/camera.hpp"

#include <entities/entity_system.hpp>

#include <input/callback_context.hpp>
#include <input/input_context.hpp>
#include <reflection/reflection.hpp>

namespace aln
{

class IComponent;

class EditorCameraController : IEntitySystem
{
    ALN_REGISTER_TYPE();

  public:
    EditorCameraController()
    {
        m_requiredUpdatePriorities.SetPriorityForStage(UpdateStage::FrameStart, 1);
    }

    void RegisterComponent(IComponent* pComponent) override;
    void UnregisterComponent(IComponent* pComponent) override;
    void Update(const UpdateContext& context) override;

  private:
    aln::input::InputContext m_inputContext;

    float m_rotationSensitivity = 0.1f;
    float m_translationSensitivity = 0.006f;
    Camera* m_pCameraInstance = nullptr;

    bool m_hasChanged = true; // default to true to trigger camera alignment on startup

    /// @brief Translate the camera position on the 2D plane parallel to the screen.
    void Move(aln::input::CallbackContext context);
    void Rotate(aln::input::CallbackContext context);
    void Zoom(aln::input::CallbackContext context);
};
} // namespace aln