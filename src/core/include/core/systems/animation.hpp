#pragma once

#include <entities/entity_system.hpp>
#include <entities/spatial_component.hpp>
#include <reflection/reflection.hpp>

#include "../../time_system.hpp"
#include "../components/animation_graph.hpp"
#include "../components/skeletal_mesh.hpp"

namespace aln
{

class AnimationSystem : public entities::IEntitySystem
{
    ALN_REGISTER_TYPE();

    AnimationGraphComponent* m_pAnimationGraphComponent;
    SkeletalMeshComponent* m_pSkeletalMeshComponent;

    // TODO: Runtime state:
    // Bone mapping table between anim skeleton and mesh skeleton

  public:
    ScriptSystem()
    {
        m_requiredUpdatePriorities.SetPriorityForStage(UpdateStage::FrameStart, 10);
    }

    // TODO: Hide UpdateContext from users
    void Update(const aln::entities::UpdateContext& ctx) override;
    void RegisterComponent(aln::entities::IComponent* pComponent) override;
    void UnregisterComponent(aln::entities::IComponent* pComponent) override;
};
} // namespace aln