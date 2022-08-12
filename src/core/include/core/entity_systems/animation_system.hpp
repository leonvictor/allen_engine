#pragma once

#include <entities/entity_system.hpp>
#include <entities/spatial_component.hpp>
#include <reflection/reflection.hpp>

#include "../components/animation_graph.hpp"
#include "../components/animation_player_component.hpp"
#include "../components/skeletal_mesh_component.hpp"
#include "../time_system.hpp"

#include <anim/skeleton.hpp>

#include <map>

namespace aln
{

class AnimationSystem : public entities::IEntitySystem
{
    ALN_REGISTER_TYPE();

    AnimationGraphComponent* m_pAnimationGraphComponent;
    AnimationPlayerComponent* m_pAnimationPlayerComponent;
    SkeletalMeshComponent* m_pSkeletalMeshComponent;

  public:
    AnimationSystem()
    {
        m_requiredUpdatePriorities.SetPriorityForStage(UpdateStage::FrameStart, 10);
    }

    // TODO: Hide UpdateContext from users
    void Update(const aln::entities::UpdateContext& ctx) override;
    void RegisterComponent(aln::entities::IComponent* pComponent) override;
    void UnregisterComponent(aln::entities::IComponent* pComponent) override;
};
} // namespace aln