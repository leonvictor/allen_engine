#pragma once

#include <entities/entity_system.hpp>
#include <entities/spatial_component.hpp>
#include <reflection/reflection.hpp>

#include "time_system.hpp"

namespace aln
{

/// @todo For now this is a test system...
class ScriptSystem : public entities::IEntitySystem
{
    ALN_REGISTER_TYPE();

    aln::entities::SpatialComponent* m_pRootComponent;

    float m_rotationSpeedX = 15.0f;
    float m_rotationSpeedY = 0.0f;
    float m_rotationSpeedZ = 0.0f;

  public:
    ScriptSystem()
    {
        m_requiredUpdatePriorities.SetPriorityForStage(UpdateStage::PostPhysics, 10);
    }

    // TODO: Hide UpdateContext from users
    void Update(const aln::entities::UpdateContext& ctx) override;

    void RegisterComponent(aln::entities::IComponent* pComponent) override;

    void UnregisterComponent(aln::entities::IComponent* pComponent) override;
};
} // namespace aln