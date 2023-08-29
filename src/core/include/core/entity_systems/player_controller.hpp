#pragma once

#include "script.hpp"

#include "../components/animation_graph.hpp"
#include "../components/skeletal_mesh_component.hpp"

namespace aln
{
class PlayerControllerSystem : public ScriptSystem
{
    ALN_REGISTER_TYPE();

    AnimationGraphComponent* m_pGraphComponent = nullptr;
    SkeletalMeshComponent* m_pCharacterMeshComponent = nullptr;

    float m_blendWeight = 0.0f;
    NodeIndex m_blendWeightParameterIndex = InvalidIndex;

    public:
        PlayerControllerSystem()
        {
            m_requiredUpdatePriorities.SetPriorityForStage(UpdateStage::PostPhysics, 1);
        }

    void Update(const UpdateContext& ctx) override;

    void RegisterComponent(IComponent* pComponent) override;

    void UnregisterComponent(IComponent* pComponent) override;

};
} // namespace aln