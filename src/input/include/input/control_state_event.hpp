#pragma once

/// Forward declarations
class IInputControl;

/// @brief Event launched by devices each time a control changes.
/// To be consumed/dispatched  by the input system
struct ControlStateChangedEvent
{
    // Should control be copied ? We don't want to access the wrong value later on
    // Value should be accessed from control directly to avoid having to template this
    const IInputControl* pControl;

    // TODO: Time emitted
};