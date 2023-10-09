#pragma once

namespace aln
{
/// Forward declarations
class IInputControl;

/// @brief Event launched by devices each time a control changes.
/// To be consumed/dispatched  by the input system
struct ControlStateChangedEvent
{
    const IInputControl* m_pControl = nullptr;
    // TODO: Time emitted
};
} // namespace aln
