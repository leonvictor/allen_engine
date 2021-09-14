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

  public:
    // TODO: Hide UpdateContext from users
    void Update(const aln::entities::UpdateContext& ctx) override
    {
        auto transform = m_pRootComponent->ModifyTransform();
        transform->rotation *= Time::GetDeltaTime();
    }

    void RegisterComponent(aln::entities::IComponent* pComponent) override
    {
        auto pSpatialComponent = dynamic_cast<aln::entities::SpatialComponent*>(pComponent);
        if (pSpatialComponent != nullptr)
        {
            m_pRootComponent = pSpatialComponent;
        }
    }

    void UnregisterComponent(aln::entities::IComponent* pComponent) override
    {
        if (pComponent == m_pRootComponent)
        {
            m_pRootComponent = nullptr;
        }
    }
};
} // namespace aln