#pragma once

#include <common/services/service_provider.hpp>
#include <common/types.hpp>
#include <common/update_stages.hpp>

#include <cstdint>
namespace aln
{
class Engine;

// fwd
class Entity;
class IComponent;
class EntityMap;

/// @brief Data structure containing info on the current context.
/// Will be propagated through the object model, entities and systems will act depending on it.
class UpdateContext
{
    friend class Engine;

  private:
    ServiceProvider* m_pServiceProvider = nullptr;

    UpdateStage m_updateStage = UpdateStage::FrameStart;

    // TODO: Move to Viewport class
    float m_displayWidth = 0.0f;
    float m_displayHeight = 0.0f;

    // Time data
    Seconds m_deltaTime = 0.0f;
    Seconds m_currentTime = 0.0f;
    Seconds m_timeSinceAppStart = 0.0f;

  public:
    /// @todo Should be private
    EntityMap* pEntityMap;

    UpdateStage GetUpdateStage() const { return m_updateStage; }
    
    /// @brief Time delta between this frame and the last
    inline Seconds GetDeltaTime() const { return m_deltaTime; }
    /// @brief Time at the start of this frame
    inline Seconds GetTime() const { return m_currentTime; }
    /// @brief Time elapsed since application start
    inline Seconds GetTimeSinceAppStart() const { return m_timeSinceAppStart; }

    inline float GetDisplayHeight() const { return m_displayHeight; }
    inline float GetDisplayWidth() const { return m_displayWidth; }

    template <typename T>
    T* GetService() const
    {
        assert(m_pServiceProvider != nullptr);
        return m_pServiceProvider->GetService<T>();
    }
};
} // namespace aln