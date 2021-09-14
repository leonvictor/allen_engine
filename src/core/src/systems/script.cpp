#include "systems/script.hpp"

namespace aln
{
void ScriptSystem::Update(const aln::entities::UpdateContext& ctx)
{
    auto& transform = m_pRootComponent->ModifyTransform();
    transform->rotation.x += m_rotationSpeedX * Time::GetDeltaTime();
    transform->rotation.y += m_rotationSpeedY * Time::GetDeltaTime();
    transform->rotation.z += m_rotationSpeedZ * Time::GetDeltaTime();
}

void ScriptSystem::RegisterComponent(aln::entities::IComponent* pComponent)
{
    auto pSpatialComponent = dynamic_cast<aln::entities::SpatialComponent*>(pComponent);
    if (pSpatialComponent != nullptr)
    {
        m_pRootComponent = pSpatialComponent;
    }
}

void ScriptSystem::UnregisterComponent(aln::entities::IComponent* pComponent)
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