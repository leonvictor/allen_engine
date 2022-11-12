#include "entity_systems/script.hpp"

namespace aln
{
void ScriptSystem::Update(const UpdateContext& ctx)
{
    // TODO: Register systems such as Time and Input, and provide derived classes with getters methods like GetTime() or KeyPressed()
    // At the same time, get rid of the Time singleton
    // Also make it pseudo-realtime
    auto rot = m_pRootComponent->GetLocalTransform().GetRotationEuler();
    rot.x += m_rotationSpeedX * Time::GetDeltaTime();
    rot.y += m_rotationSpeedY * Time::GetDeltaTime();
    rot.z += m_rotationSpeedZ * Time::GetDeltaTime();
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
ALN_REFLECT_MEMBER(m_rotationSpeedX)
ALN_REFLECT_MEMBER(m_rotationSpeedY)
ALN_REFLECT_MEMBER(m_rotationSpeedZ)
ALN_REGISTER_IMPL_END()
} // namespace aln