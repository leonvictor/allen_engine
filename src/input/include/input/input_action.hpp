#pragma once

#include "input_action_phase.hpp"
#include "interactions.hpp"

#include <functional>
#include <memory>

namespace aln
{
/// Forward declaration
class IInputControl;
class Interaction;
class CallbackContext;

/// @brief Represent a logical action following an input, "jump", "move camera"
class InputAction
{
    friend class InputContext;
    struct Context
    {
        const IInputControl* pControl = nullptr;
    };

  private:
    std::function<void(CallbackContext)> m_callback;
    InputActionPhase m_phase = InputActionPhase::Disabled;
    Interaction::Type m_interactionType = Interaction::Type::Press;
    std::shared_ptr<Interaction> m_pInteraction;

  public:
    void SetCallback(std::function<void(CallbackContext)> callback) { m_callback = callback; }
    void SetInteraction(Interaction::Type interactionType) { m_interactionType = interactionType; }

    void Trigger(InputAction::Context actionContext);

    inline void Enable() { m_phase = Waiting; }
    inline void Disable() { m_phase = Disabled; }
    inline bool IsEnabled() { return m_phase != Disabled; }

    /// @brief Whether the action is neither disabled or waiting.
    inline bool IsActive() { return IsEnabled() && m_phase != Waiting; }
};
}