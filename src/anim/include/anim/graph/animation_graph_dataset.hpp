#pragma once

#include "../animation_clip.hpp"

namespace aln
{
/// @brief "Argument" to the "Graph" function.
/// It is a flat list of every AnimationClipNode in the graph
/// Used to separate data from logic

class AnimationGraphDataSet
{
  private:
    std::string m_name = "Default";

  public:
    // TODO: Allow derivation
    std::vector<AnimationGraphDataSet*> m_derivations;

    // TODO
    /// Map node to actual resource
    /// Jog L -> Data/JogL.anim

    AnimationClip* GetAnimationClip(uint32_t index) const
    {
        // TODO
    }

    // TODO: Instanciation:
    AnimationGraphDataSet Derive(std::string name) const
    {
    }
};
} // namespace aln