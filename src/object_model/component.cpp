#include "component.hpp"

#include <assert.h>
#include <stdexcept>

bool IComponent::LoadComponentAsync()
{
    // Loading must be running or not started yet.
    assert(m_status == Status::Unloaded || m_status == Status::Loading);

    if (m_status == Status::Unloaded)
    {
        // Start loading
        m_status = Status::Loading;
        m_loadingTask = std::async(std::launch::async, &IComponent::Load, this);
        // Do not return right away in case loading is super fast
    }

    // Check if the loading function returned
    if (m_loadingTask.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
    {
        // Set the status accordingly
        if (m_loadingTask.get())
        {
            m_status = Status::Loaded;
            return true;
        }
        else
        {
            m_status = Status::LoadingFailed;
            return false;
        }
    }

    // Still loading
    return false;
}

void IComponent::InitializeComponent()
{
    assert(m_status == Status::Loaded);
    Initialize();
    m_status == Status::Initialized;
}

void IComponent::UnloadComponent()
{
    assert(m_status == Status::Loaded || m_status == Status::LoadingFailed);
    Unload();
    m_status = Status::Unloaded;
}

void IComponent::ShutdownComponent()
{
    assert(m_status == Status::Initialized);
    Shutdown();
    m_status = Status::Loaded;
}