#pragma once

#include "callback_context.hpp"
#include "input_action_phase.hpp"
#include "interactions.hpp"

/// Forward declaration
class IInputControl;

/// @brief: Represent a logical action following an input, "jump", "move camera"
class InputAction
{
    friend class InputContext;
    struct Context
    {
        const IInputControl* pControl;
    };

  private:
    std::function<void(CallbackContext)> m_callback;
    InputActionPhase m_phase = InputActionPhase::Disabled;
    Interaction::Type m_interactionType = Interaction::Type::Press;
    std::shared_ptr<Interaction> m_pInteraction;

  public:
    void SetCallback(std::function<void(CallbackContext)> callback) { m_callback = callback; }
    void SetInteraction(Interaction::Type interactionType) { m_interactionType = interactionType; }

    void Trigger(InputAction::Context actionContext)
    {
        // Do nothing if the action is not enabled.
        if (!IsEnabled())
        {
            return;
        }

        // Start interaction if needed
        if (m_pInteraction == nullptr)
        {
            // TODO: Move to a factory
            switch (m_interactionType)
            {
            case Interaction::Type::Press:
                // TODO
                break;
            case Interaction::Type::Hold:
                m_pInteraction = std::make_shared<HoldInteraction>();
                break;
            }
        }

        Interaction::Context interactionContext = {
            .pAction = this,
            .pControl = actionContext.pControl,
            .phase = m_phase,
        };

        // Query the interaction for the current phase
        m_phase = m_pInteraction->Update(interactionContext);

        CallbackContext callbackContext = {
            // TODO: m_interaction
        };

        // TODO: Different callbacks based on the phase.
        switch (m_phase)
        {
        case InputActionPhase::Waiting:
            break;

        case InputActionPhase::Started:
            // TODO: Custom callback
            break;

        case InputActionPhase::Performed:
            m_callback(callbackContext);
            break;

        case InputActionPhase::Canceled:
            // TODO: specific callback

            // Start waiting again
            m_phase = InputActionPhase::Waiting;
            break;
        }
    }

    void Enable() { m_phase = Waiting; }
    void Disable() { m_phase = Disabled; }
    bool IsEnabled() { return m_phase != Disabled; }

    /// @brief Whether the action is neither disabled or waiting.
    bool IsActive() { return IsEnabled() && m_phase != Waiting; }
};