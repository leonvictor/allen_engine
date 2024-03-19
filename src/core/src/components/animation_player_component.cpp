#include "components/animation_player_component.hpp"

#include <common/maths/maths.hpp>

namespace aln
{

ALN_REGISTER_IMPL_BEGIN(COMPONENTS, AnimationPlayerComponent)
ALN_REFLECT_MEMBER(m_percentageThroughAnimation)
ALN_REFLECT_MEMBER(m_pause)
ALN_REFLECT_MEMBER(m_pAnimationClip)
ALN_REGISTER_IMPL_END()

void AnimationPlayerComponent::Update(Seconds deltaTime)
{
    if (!m_pause)
    {
        // TODO switch on play mode
        m_previousPercentageThroughAnimation = m_percentageThroughAnimation;
        m_percentageThroughAnimation += (deltaTime / m_pAnimationClip->GetDuration());

        // Loop
        m_percentageThroughAnimation = Maths::Mod(m_percentageThroughAnimation, 1.0f);
    }

    m_pAnimationClip->GetPose(m_percentageThroughAnimation, m_pPose);
    m_pPose->CalculateGlobalTransforms();
}
} // namespace aln