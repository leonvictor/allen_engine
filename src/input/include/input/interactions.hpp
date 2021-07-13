#pragma once

#include "controls/input_control.hpp"
#include "input_action_phase.hpp"

namespace aln::input
{
/// Forward declarations
class InputAction;

/// @brief Represent an active interaction between the controls and the actions.
/// Interactions can be stateful
/// TODO: Rename IInteraction
class Interaction
{
  public:
    struct Context
    {
        const InputAction* pAction;
        const IInputControl* pControl;
        InputActionPhase phase;

        // TODO: Time of the control change that triggered the input
        // TODO: Time at which the Interaction was first called ?
    };

    enum Type
    {
        Press,
        Hold,
    };

    /// @brief Called everytime the associated control change value.
    /// @return The phase the interaction is left at.
    virtual InputActionPhase Update(Interaction::Context interactionContext) = 0;

  protected:
    Type m_type;
};

/// @brief Start the action after the control is first pressed, perform it
/// as long as the key is pressed, cancel when it is released.
class HoldInteraction : public Interaction
{
    bool m_held = false;

  public:
    HoldInteraction()
    {
        m_type = Type::Hold;
    }

    InputActionPhase Update(Interaction::Context interactionContext)
    {
        if (interactionContext.pControl->IsActuated())
        {
            if (m_held)
            {
                return InputActionPhase::Performed;
            }
            else
            {
                m_held = true;
                return InputActionPhase::Started;
            }
        }
        else
        {
            if (m_held)
            {
                m_held = false;
                return InputActionPhase::Canceled;
            }
            else
            {
                return InputActionPhase::Waiting;
            }
        }
    }
};

/// @brief Fire the action after the control is released.
class PressInteraction : public Interaction
{
    //   public:
    //     PressInteraction()
    //     {
    //         m_type = Type::Press;
    //     }

    //     InputActionPhase Update(Interaction::Context interactionContext)
    //     {
    //         if (interactionContext.pControl->IsActuated())
    //         {
    //             m_held = true;
    //             return InputActionPhase::Started;
    //         }
    //         else
    //         {
    //             if (m_held)
    //             {
    //                 m_held = false;
    //                 return InputActionPhase::Performed;
    //             }
    //         }

    //         return interactionContext.phase;
    //     }
};
} // namespace aln::input