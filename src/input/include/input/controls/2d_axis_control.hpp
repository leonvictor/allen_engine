#pragma once

#include "input_control.hpp"

#include <glm/vec2.hpp>

namespace aln
{

/// @brief 2D floating point axis control (i.e. thumbstick)
class AxisControl2D : public InputControl<glm::vec2>
{
    public :
        AxisControl2D() : InputControl<glm::vec2>({0.0f, 0.0f}){}
        void Update() override{}

};
} // namespace aln