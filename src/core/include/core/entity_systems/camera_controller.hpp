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
    float m_rotationSensitivity = 0.1f;
    float m_translationSensitivity = 0.006f;
    Camera* m_pCameraInstance = nullptr;
};
} // namespace aln