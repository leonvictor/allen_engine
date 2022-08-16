#include "component.hpp"

#include <assert.h>
#include <stdexcept>

namespace aln::entities
{
void IComponent::LoadComponent(const LoadingContext& loadingContext)
{
    assert(m_status == Status::Unloaded);
    Load(loadingContext);
    m_status = Status::Loading;
}

void IComponent::InitializeComponent()
{
    assert(m_status == Status::Loaded);
    Initialize();
    m_status = Status::Initialized;
}

void IComponent::UnloadComponent(const LoadingContext& loadingContext)
{
    assert(m_status == Status::Loaded || m_status == Status::LoadingFailed);
    Unload(loadingContext);
    m_status = Status::Unloaded;
}

void IComponent::ShutdownComponent()
{
    assert(m_status == Status::Initialized);
    Shutdown();
    m_status = Status::Loaded;
}
} // namespace aln::entities

ALN_REGISTER_IMPL_BEGIN(BASE, aln::entities::IComponent)
ALN_REGISTER_IMPL_END()