#include "world_system.hpp"

#include <assert.h>

namespace aln
{

void IWorldSystem::InitializeSystem()
{
    assert(m_status == Status::Uninitialized);
    Initialize();
    m_status = Status::Initialized;
}

void IWorldSystem::ShutdownSystem()
{
    assert(m_status == Status::Initialized);
    Shutdown();
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