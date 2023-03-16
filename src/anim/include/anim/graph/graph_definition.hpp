#pragma once

#include <assets/asset.hpp>
#include <assets/handle.hpp>

#include <common/serialization/binary_archive.hpp>
#include <reflection/type_descriptor.hpp>
#include <reflection/type_info.hpp>

#include "animation_graph_dataset.hpp"
#include "runtime_graph_node.hpp"

#include <vector>

namespace aln
{

/// @brief The definition of an animation graph represents all of its nodes' settings in a contiguous array.
/// Multiple Instances can refer to the same definition and share the settings' memory
/// @todo Holds info on required memory size for node instances and a vector of nodes offsets
class AnimationGraphDefinition : public IAsset
{
    ALN_REGISTER_ASSET_TYPE("agdf");

    friend class AnimationGraphEditor;
    friend class AnimationGraphCompilationContext;
    friend class AnimationGraphDefinitionLoader;
    friend class RuntimeAnimationGraphInstance;

  private:
    std::vector<RuntimeGraphNode::Settings*> m_nodeSettings;
    std::vector<NodeIndex> m_nodeIndices;
    NodeIndex m_rootNodeIndex = InvalidIndex;

    // Memory info used to instanciate the runtime node array(s)
    std::vector<uint32_t> m_nodeOffsets;
    size_t m_requiredMemorySize;
    size_t m_requiredMemoryAlignement;

  public:
    size_t GetNumNodes() const { return m_nodeSettings.size(); }
};
} // namespace aln