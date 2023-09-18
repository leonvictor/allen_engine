#include "entity_systems/player_controller.hpp"

#include <input/input_service.hpp>

namespace aln
{

ALN_REGISTER_IMPL_BEGIN(SYSTEMS, PlayerControllerSystem)
ALN_REFLECT_MEMBER(m_blendWeight)
ALN_REGISTER_IMPL_END()

void PlayerControllerSystem::Update(const UpdateContext& ctx)
{
    // TODO: Use our own math lib
    // TODO: Provide easier access to services in user-facing base script class
    const auto leftStickState = ctx.GetService<InputService>()->GetGamepad()->GetLeftStickValue();
    auto speed = leftStickState.SquaredMagnitude();
    m_blendWeight = speed;

    m_pGraphComponent->SetControlParameterValue(m_blendWeightParameterIndex, m_blendWeight);

    // TODO: This should be the responsibility of the animation system. Use that when system update priorities are functionnal !
    // TODO: Ensure we move the root spatial component
    const auto& characterWorldTransform = m_pCharacterMeshComponent->GetWorldTransform();
    const auto& rootMotionDelta = m_pGraphComponent->GetRootMotionDelta();

    const auto deltaRotation = rootMotionDelta.GetRotation();
    auto deltaTranslation = rootMotionDelta.GetTranslation();
    deltaTranslation = characterWorldTransform.RotateVector(deltaTranslation);
    deltaTranslation = characterWorldTransform.ScaleVector(deltaTranslation);

    auto transform = m_pCharacterMeshComponent->GetLocalTransform();
    transform.AddRotation(deltaRotation);
    transform.AddTranslation(deltaTranslation);
    m_pCharacterMeshComponent->SetLocalTransform(transform);
}

void PlayerControllerSystem::RegisterComponent(IComponent* pComponent)
{
    auto pGraphComponent = dynamic_cast<AnimationGraphComponent*>(pComponent);
    if (pGraphComponent != nullptr)
    {
        m_pGraphComponent = pGraphComponent;
        m_blendWeightParameterIndex = pGraphComponent->GetControlParameterIndex("Speed");
        return;
    }

    auto pSkeletalMeshComponent = dynamic_cast<SkeletalMeshComponent*>(pComponent);
    if (pSkeletalMeshComponent != nullptr)
    {
        m_pCharacterMeshComponent = pSkeletalMeshComponent;
        return;
    }
}

void PlayerControllerSystem::UnregisterComponent(IComponent* pComponent)
{
    if (pComponent == m_pGraphComponent)
    {
        m_pGraphComponent = nullptr;
    }

    if (pComponent == m_pCharacterMeshComponent)
    {
        m_pCharacterMeshComponent = nullptr;
    }
}
} // namespace aln