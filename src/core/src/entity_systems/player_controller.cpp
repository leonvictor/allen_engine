#include "entity_systems/player_controller.hpp"

namespace aln
{

ALN_REGISTER_IMPL_BEGIN(SYSTEMS, PlayerControllerSystem)
ALN_REFLECT_MEMBER(m_blendWeight, Blend Weight)
ALN_REGISTER_IMPL_END()

void PlayerControllerSystem::Update(const UpdateContext& ctx)
{
    m_pGraphComponent->SetControlParameterValue(m_blendWeightParameterIndex, m_blendWeight);
}

void PlayerControllerSystem::RegisterComponent(IComponent* pComponent)
{
    auto pGraphComponent = dynamic_cast<AnimationGraphComponent*>(pComponent);
    if (pGraphComponent != nullptr)
    {
        m_pGraphComponent = pGraphComponent;
        m_blendWeightParameterIndex = pGraphComponent->GetControlParameterIndex("BlendWeight");
        return;
    }
}

void PlayerControllerSystem::UnregisterComponent(IComponent* pComponent)
{
    if (pComponent == m_pGraphComponent)
    {
        m_pGraphComponent = nullptr;
    }
}
} // namespace aln