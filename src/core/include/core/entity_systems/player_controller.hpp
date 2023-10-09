#pragma once

#include "script.hpp"

#include <anim/types.hpp>

namespace aln
{
class OrbitCameraComponent;
class AnimationGraphComponent;
class SkeletalMeshComponent;

class PlayerControllerSystem : public ScriptSystem
{
    ALN_REGISTER_TYPE();

    AnimationGraphComponent* m_pGraphComponent = nullptr;
    SkeletalMeshComponent* m_pCharacterMeshComponent = nullptr;
    OrbitCameraComponent* m_pCameraComponent = nullptr;

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