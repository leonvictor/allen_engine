#pragma once

#include "camera.hpp"
#include <entities/entity_system.hpp>

#include <input/callback_context.hpp>
#include <input/input_context.hpp>
#include <reflection/reflection.hpp>

namespace aln
{
namespace entities
{
class IComponent;
}

class EditorCameraController : aln::entities::IEntitySystem
{
    ALN_REGISTER_TYPE();

  public:
    void RegisterComponent(aln::entities::IComponent* pComponent);
    void UnregisterComponent(aln::entities::IComponent* pComponent);

  private:
    aln::input::InputContext m_inputContext;

    float m_rotationSensitivity = 0.1f;
    float m_translationSensitivity = 0.006f;
    Camera* m_pCameraInstance;

    /// @brief Translate the camera position on the 2D plane parallel to the screen.
    void Move(aln::input::CallbackContext context);
    void Rotate(aln::input::CallbackContext context);
    void Zoom(aln::input::CallbackContext context);
};
} // namespace aln