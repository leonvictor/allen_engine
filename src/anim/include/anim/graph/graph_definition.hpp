#pragma once

#include <assets/asset.hpp>
#include <assets/handle.hpp>

#include "animation_graph_dataset.hpp"
#include "runtime_graph_node.hpp"

#include <vector>

namespace aln
{

/// @brief The definition of an animation graph represents all of its nodes' settings in a contiguous array.
/// It is created from editor only nodes via a compile step
class AnimationGraphDefinition : public IAsset
{
    ALN_REGISTER_ASSET_TYPE("agdf");

    friend class AnimationGraphCompilationContext;
    friend class AnimationGraphDefinitionLoader;
    friend class RuntimeGraphInstance;
    // TODO: Holds info on required memory size for node instances and a vector of nodes offsets

  private:
    /// @todo Graph definitions can be associated with different anim datasets
    AssetHandle<AnimationGraphDataset> m_pDataset;

    /// @todo Should be contiguous
    std::vector<RuntimeGraphNode::Settings*> m_nodeSettings;

  public:
    size_t GetNumNodes() const { return m_nodeSettings.size(); }
};
} // namespace aln