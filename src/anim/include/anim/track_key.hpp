#pragma once

#include <common/transform.hpp>

namespace aln
{
/// @brief Keys composing an animation track
/// @note For now our keys contain a full transform.
/// @todo We will probably need to split them per-component later on...
struct TrackKey
{
    float m_time; // TODO: Quantize
    Transform m_component;
};
} // namespace aln