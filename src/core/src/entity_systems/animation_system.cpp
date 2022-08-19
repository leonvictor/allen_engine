#include "entity_systems/animation_system.hpp"

namespace aln
{
void AnimationSystem::Update(const aln::entities::UpdateContext& ctx)
{
    m_pAnimationPlayerComponent->Update(ctx.GetDeltaTime());
    m_pSkeletalMeshComponent->SetPose(m_pAnimationPlayerComponent->GetPose());
    // m_pSkeletalMeshComponent->ResetPoseSkeleton();
    // m_pSkeletalMeshComponent->ResetPose();
    // TODO:
    // - Update all remaining bones (procedural etc)
}

void AnimationSystem::RegisterComponent(aln::entities::IComponent* pComponent)
{
    auto pAnimationPlayerComponent = dynamic_cast<AnimationPlayerComponent*>(pComponent);
    if (pAnimationPlayerComponent != nullptr)
    {
        m_pAnimationPlayerComponent = pAnimationPlayerComponent;
        return;
    }

    auto pSkeletalMesh = dynamic_cast<SkeletalMeshComponent*>(pComponent);
    if (pSkeletalMesh != nullptr)
    {
        m_pSkeletalMeshComponent = pSkeletalMesh;
        return;
    }
}

void AnimationSystem::UnregisterComponent(aln::entities::IComponent* pComponent)
{
    if (pComponent == m_pSkeletalMeshComponent)
    {
        m_pSkeletalMeshComponent = nullptr;
    }
    else if (pComponent == m_pAnimationPlayerComponent)
    {
        m_pAnimationPlayerComponent = nullptr;
    }
}

ALN_REGISTER_IMPL_BEGIN(SYSTEMS, aln::AnimationSystem)
ALN_REGISTER_IMPL_END()
} // namespace aln