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
class AnimationGraphDefinition : public IAsset
{
    ALN_REGISTER_ASSET_TYPE("agdf");

    friend class EditorAnimationGraph;
    friend class AnimationGraphCompilationContext;
    friend class AnimationGraphDefinitionLoader;
    friend class RuntimeAnimationGraphInstance;

  private:
    std::vector<RuntimeGraphNode::Settings*> m_nodeSettings;
    std::vector<NodeIndex> m_nodeIndices;
    std::vector<StringID> m_controlParameterNames;
    NodeIndex m_rootNodeIndex = InvalidIndex;

    // Memory info used to instanciate the runtime node array(s)
    std::vector<uint32_t> m_nodeOffsets;
    size_t m_requiredMemorySize;
    size_t m_requiredMemoryAlignement;

  public:
    AnimationGraphDefinition() = default;

    // Disable copy/move
    AnimationGraphDefinition(const AnimationGraphDefinition&) = delete;
    AnimationGraphDefinition(AnimationGraphDefinition&&) = delete;
    AnimationGraphDefinition& operator=(const AnimationGraphDefinition&) = delete;
    AnimationGraphDefinition& operator=(AnimationGraphDefinition&&) = delete;
        
    size_t GetNumNodes() const { return m_nodeSettings.size(); }

    template <typename Archive>
    void Serialize(Archive& archive) const
    {
        archive << m_nodeIndices;
        archive << m_controlParameterNames;
        archive << m_rootNodeIndex;
        archive << m_nodeOffsets;
        archive << m_requiredMemorySize;
        archive << m_requiredMemoryAlignement;
    }

    template<typename Archive>
    void Deserialize(Archive& archive)
    {
        archive >> m_nodeIndices;
        archive >> m_controlParameterNames;
        archive >> m_rootNodeIndex;
        archive >> m_nodeOffsets;
        archive >> m_requiredMemorySize;
        archive >> m_requiredMemoryAlignement;
    }
};

static_assert(!std::is_trivially_copyable_v<AnimationGraphDefinition>);
} // namespace aln