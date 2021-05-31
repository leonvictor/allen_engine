#pragma once

enum InputActionPhase
{
    Disabled,  // Not enabled.
    Waiting,   // Enabled, waiting for input.
    Started,   // Associated control has been actuated.
    Performed, // Associated control successfuly reached the end of the expected sequence.
    Canceled   // Associated control has not successfuly reached the end of the expected sequence (ex: not held long enough).
};