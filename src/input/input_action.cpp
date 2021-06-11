#include "input_action.hpp"

#include "callback_context.hpp"
#include "input_action_phase.hpp"

void InputAction::Trigger(InputAction::Context actionContext)
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