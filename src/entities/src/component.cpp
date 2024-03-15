#include "component.hpp"

#include <assert.h>
#include <stdexcept>

namespace aln
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
    assert(m_status != Status::Unloaded);
    Unload(loadingContext);
    m_status = Status::Unloaded;
}

void IComponent::ShutdownComponent()
{
    assert(m_status == Status::Initialized);
    Shutdown();
    m_status = Status::Loaded;
}
} // namespace aln