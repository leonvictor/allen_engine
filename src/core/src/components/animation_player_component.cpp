#include "components/animation_player_component.hpp"

#include <common/maths/maths.hpp>

namespace aln
{

ALN_REGISTER_IMPL_BEGIN(COMPONENTS, AnimationPlayerComponent)
ALN_REFLECT_MEMBER(m_animTime)
ALN_REFLECT_MEMBER(m_pause)
ALN_REFLECT_MEMBER(m_pAnimationClip)
ALN_REGISTER_IMPL_END()

void AnimationPlayerComponent::Update(Seconds deltaTime)
{
    if (!m_pause)
    {
        // TODO switch on play mode
        m_previousAnimTime = m_animTime;
        m_animTime += deltaTime;

        // Loop
        m_animTime = Maths::Mod(m_animTime, m_pAnimationClip->GetDuration());
    }

    m_pAnimationClip->GetPose(m_animTime, m_pPose);
    m_pPose->CalculateGlobalTransforms();
}
} // namespace aln