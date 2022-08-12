#pragma once

#include <common/transform.hpp>

namespace aln
{
/// @brief Keys composing an animation track
template <typename T>
struct TrackKey
{
    float m_time; // TODO: Quantize
    T m_component;
};
} // namespace aln