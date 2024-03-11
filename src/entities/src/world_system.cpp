#include "world_system.hpp"

#include "common/services/service_provider.hpp"

#include <assert.h>

namespace aln
{

void IWorldSystem::InitializeSystem(const ServiceProvider& serviceProvider)
{
    assert(m_status == Status::Uninitialized);
    Initialize(serviceProvider);
    m_status = Status::Initialized;
}

void IWorldSystem::ShutdownSystem(const ServiceProvider& serviceProvider)
{
    assert(m_status == Status::Initialized);
    Shutdown(serviceProvider);
    m_status = Status::Uninitialized;
}

void IWorldSystem::Enable()
{
    assert(m_status == Status::Initialized);
    m_status = Status::Enabled;
}

void IWorldSystem::Disable()
{
    assert(m_status == Status::Enabled);
    m_status = Status::Initialized;
}
}