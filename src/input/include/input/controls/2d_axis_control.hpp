#pragma once

#include "../input_control.hpp"

#include <common/maths/vec2.hpp>

namespace aln
{

/// @brief 2D floating point axis control (i.e. thumbstick)
class AxisControl2D : public InputControl<Vec2>
{
    friend class Gamepad;

    public :
        AxisControl2D() : InputControl<Vec2>({0.0f, 0.0f}){}
        void Update() override{}

};
} // namespace aln