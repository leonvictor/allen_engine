#include "components/animation_player_component.hpp"

ALN_REGISTER_IMPL_BEGIN(COMPONENTS, aln::AnimationPlayerComponent)
ALN_REFLECT_MEMBER(m_animTime, Animation Time)
ALN_REFLECT_MEMBER(m_pause, Pause)
ALN_REFLECT_MEMBER(m_pAnimationClip, Clip)
ALN_REFLECT_MEMBER(m_pSkeleton, Skeleton)
ALN_REGISTER_IMPL_END()

namespace aln
{
void AnimationPlayerComponent::Update(Seconds deltaTime)
{
    if (!m_pause)
    {
        // TODO switch on play mode
        m_previousAnimTime = m_animTime;
        m_animTime += deltaTime;

        // Loop
        m_animTime = std::fmod(m_animTime, m_pAnimationClip->GetDuration());
    }

    m_pAnimationClip->GetPose(m_animTime, m_pPose);
    m_pPose->CalculateGlobalTransforms();
}
} // namespace aln