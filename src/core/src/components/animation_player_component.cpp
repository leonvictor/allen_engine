#include "components/animation_player_component.hpp"

ALN_REGISTER_IMPL_BEGIN(COMPONENTS, aln::AnimationPlayerComponent)
ALN_REFLECT_MEMBER(m_animTime)
ALN_REGISTER_IMPL_END()

namespace aln
{
void AnimationPlayerComponent::Update(Seconds deltaTime)
{
    // TODO switch on play mode
    m_previousAnimTime = m_animTime;
    m_animTime += deltaTime;
    m_animTime = std::fmod(m_animTime, m_pAnimationClip->GetDuration() / m_pAnimationClip->GetTicksPerSecond());

    auto timeInTicks = m_animTime * m_pAnimationClip->GetTicksPerSecond();

    // TODO: No longer use ticks
    m_pAnimationClip->GetPose(timeInTicks, m_pPose);
    m_pPose->CalculateGlobalTransforms();
}
} // namespace aln