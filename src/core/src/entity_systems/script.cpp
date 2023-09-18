#include "entity_systems/script.hpp"

namespace aln
{
ALN_REGISTER_IMPL_BEGIN(SYSTEMS, ScriptSystem)
ALN_REFLECT_MEMBER(m_rotationSpeedX)
ALN_REFLECT_MEMBER(m_rotationSpeedY)
ALN_REFLECT_MEMBER(m_rotationSpeedZ)
ALN_REGISTER_IMPL_END()

void ScriptSystem::Update(const UpdateContext& ctx)
{
    auto rot = m_pRootComponent->GetLocalTransform().GetRotationEuler();
    rot.x += m_rotationSpeedX * ctx.GetDeltaTime();
    rot.y += m_rotationSpeedY * ctx.GetDeltaTime();
    rot.z += m_rotationSpeedZ * ctx.GetDeltaTime();
    m_pRootComponent->SetLocalTransformRotationEuler(rot);
}

void ScriptSystem::RegisterComponent(IComponent* pComponent)
{
    auto pSpatialComponent = dynamic_cast<SpatialComponent*>(pComponent);
    if (pSpatialComponent != nullptr)
    {
        m_pRootComponent = pSpatialComponent;
    }
}

void ScriptSystem::UnregisterComponent(IComponent* pComponent)
{
    if (pComponent == m_pRootComponent)
    {
        m_pRootComponent = nullptr;
    }
}
} // namespace aln