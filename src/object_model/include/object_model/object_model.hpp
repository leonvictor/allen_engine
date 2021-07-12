#pragma once

#include <cstdint>

#include "../update_stages.hpp"

class Entity;
class IComponent;

// TODO: Get rid of this namespace.
namespace ObjectModel
{

/// @brief Data structure containing info on the current context.
/// Will be propagated through the object model, entities and systems will act depending on it.
/// @note This is why it's a class, we don't want to update the current stage mid-flight for example.
/// @todo I think it'd be better to just friend the class creating contexts (the main loop ?)
class UpdateContext
{
  private:
    UpdateStage m_updateStage;

  public:
    uint32_t displayWidth;
    uint32_t displayHeight;

    UpdateContext(UpdateStage stage) : m_updateStage(stage) {}

    UpdateStage GetUpdateStage() const
    {
        return m_updateStage;
    }
};
} // namespace ObjectModel