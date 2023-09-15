#pragma once

#include <assets/asset.hpp>
#include <assets/handle.hpp>
#include <common/serialization/binary_archive.hpp>

#include "../animation_clip.hpp"

namespace aln
{
/// @brief "Argument" to the "Graph" function.
/// It is a flat list of every AnimationClipNode in the graph
/// Used to separate data from logic
class AnimationGraphDataset : public IAsset
{
    ALN_REGISTER_ASSET_TYPE("agds");

    friend class AnimationGraphDatasetLoader;

  private:
    std::string m_name = "Default";

  public:
    // TODO: Allow derivation
    Vector<AnimationGraphDataset*> m_derivations;

    Vector<AssetHandle<AnimationClip>> m_animationClips;

    // TODO
    /// Map node to actual resource
    /// Jog L -> Data/JogL.anim
    const AnimationClip* GetAnimationClip(uint32_t index) const
    {
        return m_animationClips[index].get();
    }

    void Serialize(BinaryMemoryArchive& archive) const
    {
        archive << m_animationClips.size();
        for (auto& clipHandle : m_animationClips)
        {
            archive << clipHandle.GetAssetID();
        }
    }

    // TODO: Instanciation:
    AnimationGraphDataset Derive(std::string name) const
    {
    }
};
} // namespace aln