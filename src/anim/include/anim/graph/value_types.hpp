#pragma once

#include <cstdint>

namespace aln
{
enum class NodeValueType : uint8_t
{
    Unknown,
    Pose,
    Bool,
    ID,
    Int,
    Float,
    Vector,
    Target,
    BoneMask,
};
}