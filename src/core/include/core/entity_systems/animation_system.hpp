#pragma once

#include <entities/entity_system.hpp>
#include <entities/spatial_component.hpp>
#include <reflection/type_info.hpp>

#include "../components/animation_graph.hpp"
#include "../components/animation_player_component.hpp"
#include "../components/skeletal_mesh_component.hpp"

#include <anim/skeleton.hpp>

namespace aln
{

class AnimationSystem : public IEntitySystem
{
    ALN_REGISTER_TYPE();

    AnimationGraphComponent* m_pAnimationGraphComponent = nullptr;
    AnimationPlayerComponent* m_pAnimationPlayerComponent = nullptr;
    SkeletalMeshComponent* m_pSkeletalMeshComponent = nullptr;

  public:
    AnimationSystem()
    {
        m_requiredUpdatePriorities.SetPriorityForStage(UpdateStage::FrameStart, 10);
    }

    void Update(const UpdateContext& ctx) override;
    void RegisterComponent(IComponent* pComponent) override;
    void UnregisterComponent(IComponent* pComponent) override;
};
} // namespace aln