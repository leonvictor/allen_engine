#pragma once

#include <entities/entity_system.hpp>
#include <entities/spatial_component.hpp>
#include <reflection/type_info.hpp>

namespace aln
{

/// @todo For now this is a test system...
/// @todo This is intended to be the interface from which user-provided scripts inherit
class ScriptSystem : public IEntitySystem
{
    ALN_REGISTER_TYPE();

    SpatialComponent* m_pRootComponent = nullptr;

    float m_rotationSpeedX = 15.0f;
    float m_rotationSpeedY = 0.0f;
    float m_rotationSpeedZ = 0.0f;

  public:
    ScriptSystem()
    {
      m_requiredUpdatePriorities.SetPriorityForStage(UpdateStage::PostPhysics, 10);
    }

    // TODO: Hide UpdateContext from users
    void Update(const UpdateContext& ctx) override;

    void RegisterComponent(IComponent* pComponent) override;

    void UnregisterComponent(IComponent* pComponent) override;
};
} // namespace aln