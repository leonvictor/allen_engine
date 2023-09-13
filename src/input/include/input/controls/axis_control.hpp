#pragma once

#include "../input_control.hpp"

namespace aln
{

/// @brief Floating point axis control.
class AxisControl : public InputControl<float>
{
    // TODO: Find a better way to do this
    friend class Mouse;
    friend class Gamepad;

  public:
    AxisControl() : InputControl<float>(0.0f) {}

    void Update() {}
};
} // namespace aln