#include "entity_systems/script.hpp"

namespace aln
{
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

ALN_REGISTER_IMPL_BEGIN(SYSTEMS, aln::ScriptSystem)
ALN_REFLECT_MEMBER(m_rotationSpeedX, Rotation Speed X)
ALN_REFLECT_MEMBER(m_rotationSpeedY, Rotation Speed Y)
ALN_REFLECT_MEMBER(m_rotationSpeedZ, Rotation Speed Z)
ALN_REGISTER_IMPL_END()
} // namespace aln