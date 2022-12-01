#pragma once

#include <cstdint>

namespace aln
{
enum class PinValueType : uint8_t
{
    None,
    Pose,
    Bool,
    Float,
    Int,
    // TODO
};
} // namespace aln
