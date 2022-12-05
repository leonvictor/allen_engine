#pragma once

#include <assets/asset.hpp>
#include <assets/handle.hpp>

#include "../animation_clip.hpp"

namespace aln
{
/// @brief "Argument" to the "Graph" function.
/// It is a flat list of every AnimationClipNode in the graph
/// Used to separate data from logic
class AnimationGraphDataset : public IAsset
{
    ALN_REGISTER_ASSET_TYPE("agds");

  private:
    std::string m_name = "Default";

  public:
    // TODO: Allow derivation
    std::vector<AnimationGraphDataset*> m_derivations;

    std::vector<AssetHandle<AnimationClip>> m_animationClips;

    // TODO
    /// Map node to actual resource
    /// Jog L -> Data/JogL.anim
    const AnimationClip* GetAnimationClip(uint32_t index) const
    {
        return m_animationClips[index].get();
    }

    // TODO: Instanciation:
    AnimationGraphDataset Derive(std::string name) const
    {
    }
};
} // namespace aln