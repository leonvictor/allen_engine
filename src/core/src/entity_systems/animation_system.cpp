#include "entity_systems/animation_system.hpp"

namespace aln
{
void AnimationSystem::Update(const UpdateContext& ctx)
{
    if (m_pSkeletalMeshComponent == nullptr)
    {
        return;
    }

    // If a graph is set, evaluate it
    if (m_pAnimationGraphComponent != nullptr)
    {
        if (m_pSkeletalMeshComponent->GetSkeleton() != m_pAnimationGraphComponent->GetPose()->GetSkeleton())
        {
            return;
        }
        m_pAnimationGraphComponent->Evaluate(ctx.GetDeltaTime(), m_pSkeletalMeshComponent->GetWorldTransform());
        m_pAnimationGraphComponent->ExecuteTasks();

        m_pSkeletalMeshComponent->SetPose(m_pAnimationGraphComponent->GetPose());
    }
    // Otherwise fall back to animation player
    else if (m_pAnimationPlayerComponent != nullptr)
    {
        if (m_pSkeletalMeshComponent->GetSkeleton() != m_pAnimationPlayerComponent->GetPose()->GetSkeleton())
        {
            return;
        }
        m_pAnimationPlayerComponent->Update(ctx.GetDeltaTime());
        m_pSkeletalMeshComponent->SetPose(m_pAnimationPlayerComponent->GetPose());
    }

    // m_pSkeletalMeshComponent->ResetPoseSkeleton();
    // m_pSkeletalMeshComponent->ResetPose();
    // TODO:
    // - Update all remaining bones (procedural etc)
}

void AnimationSystem::RegisterComponent(IComponent* pComponent)
{
    auto pAnimationPlayerComponent = dynamic_cast<AnimationPlayerComponent*>(pComponent);
    if (pAnimationPlayerComponent != nullptr)
    {
        m_pAnimationPlayerComponent = pAnimationPlayerComponent;
        return;
    }

    auto pAnimationGraphComponent = dynamic_cast<AnimationGraphComponent*>(pComponent);
    if (pAnimationGraphComponent != nullptr)
    {
        m_pAnimationGraphComponent = pAnimationGraphComponent;
        return;
    }

    auto pSkeletalMesh = dynamic_cast<SkeletalMeshComponent*>(pComponent);
    if (pSkeletalMesh != nullptr)
    {
        m_pSkeletalMeshComponent = pSkeletalMesh;
        return;
    }
}

void AnimationSystem::UnregisterComponent(IComponent* pComponent)
{
    if (pComponent == m_pSkeletalMeshComponent)
    {
        m_pSkeletalMeshComponent = nullptr;
    }
    else if (pComponent == m_pAnimationGraphComponent)
    {
        m_pAnimationGraphComponent = nullptr;
    }
    else if (pComponent == m_pAnimationPlayerComponent)
    {
        m_pAnimationPlayerComponent = nullptr;
    }
}

ALN_REGISTER_IMPL_BEGIN(SYSTEMS, aln::AnimationSystem)
ALN_REGISTER_IMPL_END()
} // namespace aln