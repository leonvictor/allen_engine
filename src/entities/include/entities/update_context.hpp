#pragma once

#include <cstdint>

#include <common/types.hpp>

#include "../update_stages.hpp"

namespace aln::entities
{

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
    UpdateStage m_updateStage = UpdateStage::FrameStart;
    Seconds m_deltaTime = 0.0f;

    // TODO: Move to Viewport class
    float m_displayWidth;
    float m_displayHeight;

  public:
    EntityMap* pEntityMap;

    // TODO: Propagate input to systems (and get rid of singletons)
    UpdateStage GetUpdateStage() const { return m_updateStage; }
    inline Seconds GetDeltaTime() const { return m_deltaTime; }
    inline Seconds GetDisplayHeight() const { return m_displayHeight; }
    inline Seconds GetDisplayWidth() const { return m_displayWidth; }
};
} // namespace aln::entities