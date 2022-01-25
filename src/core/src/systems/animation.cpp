#include "systems/animation.hpp"

namespace aln
{
void AnimationSystem::Update(const aln::entities::UpdateContext& ctx)
{
    // TODO: Update animation Component (get a pose)
    // TODO: Transfer pose to skeletal mesh
    // TODO: Update procedural bones
}

void AnimationSystem::RegisterComponent(aln::entities::IComponent* pComponent)
{
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

void AnimationSystem::UnregisterComponent(aln::entities::IComponent* pComponent)
{
    if (pComponent == m_pSkeletalMeshComponent)
    {
        m_pSkeletalMeshComponent = nullptr;
    }
    else if (pComponent == m_pAnimationGraphComponent)
    {
        m_pAnimationGraphComponent = nullptr;
    }
}

ALN_REGISTER_IMPL_BEGIN(SYSTEMS, aln::AnimationSystem)
ALN_REGISTER_IMPL_END()
} // namespace aln