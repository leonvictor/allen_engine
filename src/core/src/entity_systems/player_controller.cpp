#include "entity_systems/player_controller.hpp"

#include "components/animation_graph.hpp"
#include "components/camera.hpp"
#include "components/skeletal_mesh_component.hpp"

#include <input/input_service.hpp>

namespace aln
{

ALN_REGISTER_IMPL_BEGIN(SYSTEMS, PlayerControllerSystem)
ALN_REFLECT_MEMBER(m_blendWeight)
ALN_REGISTER_IMPL_END()

void PlayerControllerSystem::Update(const UpdateContext& ctx)
{
    // TODO: Provide easier access to services in user-facing base script class
    const auto& leftStickState = ctx.GetService<InputService>()->GetGamepad()->GetLeftStickValue();

    auto speed = leftStickState.SquaredMagnitude();
    m_blendWeight = speed;

    m_pGraphComponent->SetControlParameterValue(m_blendWeightParameterIndex, m_blendWeight);

    // TODO: This should be the responsibility of the animation system. Use that when system update priorities are functionnal !

    // TODO: Ensure we move the root spatial component
    auto pRootSpatialComponent = m_pCharacterMeshComponent;

    const auto& characterWorldTransform = pRootSpatialComponent->GetWorldTransform();
    auto characterLocalTransform = m_pCharacterMeshComponent->GetLocalTransform();

    // Apply root motion
    const auto& rootMotionDelta = m_pGraphComponent->GetRootMotionDelta();
    const auto deltaRotation = rootMotionDelta.GetRotation();
    auto deltaTranslation = rootMotionDelta.GetTranslation();

    deltaTranslation = characterWorldTransform.ScaleVector(deltaTranslation);
    deltaTranslation = characterWorldTransform.RotateVector(deltaTranslation);

    characterLocalTransform.AddTranslation(deltaTranslation);
    characterLocalTransform.AddRotation(deltaRotation);

    // Rotate the character to head where the stick is pointing to, relative to the camera
    if (!leftStickState.IsNearZero())
    {
        auto inputHeading = Vec3(-leftStickState.x, 0.0f, -leftStickState.y).Normalized();
        auto inputRotation = Quaternion::LookAt(inputHeading);
        auto cameraRotation = Quaternion::LookAt(m_cameraRelativeForwardDirection2D);
        auto rotation = cameraRotation * inputRotation;

        characterLocalTransform.SetRotation(rotation);
    }

    pRootSpatialComponent->SetLocalTransform(characterLocalTransform);

    // -- Camera
    if (m_pCameraComponent != nullptr)
    {
        // - Player-driven camera movement
        const auto& rightStickState = ctx.GetService<InputService>()->GetGamepad()->GetRightStickValue();

        // TODO: make this parametrable
        constexpr float rotationSpeed = 90.0f;

        const auto yawDelta = (Degrees) rightStickState.x * rotationSpeed * ctx.GetDeltaTime();
        const auto pitchDelta = (Degrees) rightStickState.y * rotationSpeed * ctx.GetDeltaTime();

        if (!(Maths::IsNearZero((float) yawDelta) && Maths::IsNearZero((float) pitchDelta))) // Skip small deltas
        {
            m_pCameraComponent->OffsetOrbitAngle(yawDelta, pitchDelta);
            m_lastCameraInputTime = ctx.GetTime();
        }

        // - Automatic centering after having not received camera input for a while
        //else if (ctx.GetTime() - m_lastCameraInputTime > m_autoAlignDelay)
        //{
        //    auto characterHeading = m_pCharacterMeshComponent->GetWorldTransform().GetForwardVector();
        //    auto angle = Maths::Acos(characterHeading.z).ToDegrees();
        //    angle = characterHeading.x < 0.0f ? Degrees(360.0f) - angle : angle;
        //    m_pCameraComponent->RotateHorizontallyTowards(angle, rotationSpeed * ctx.GetDeltaTime());
        //}

        // - Compute the final camera transform
        m_pCameraComponent->FinalizeUpdate(ctx.GetDeltaTime());

        auto cameraDirection2D = m_pCameraComponent->GetCameraForwardVector();
        cameraDirection2D.y = 0.0f;
        m_cameraRelativeForwardDirection2D = cameraDirection2D.Normalized();
    }
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

    auto pCameraComponent = dynamic_cast<OrbitCameraComponent*>(pComponent);
    if (pCameraComponent != nullptr)
    {
        m_pCameraComponent = pCameraComponent;
        m_pCameraComponent->SetFocusOffset(Vec3::WorldUp);
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

    if (pComponent == m_pCameraComponent)
    {
        m_pCameraComponent = nullptr;
    }
}
} // namespace aln