#pragma once

#include <assets/asset.hpp>
#include <assets/handle.hpp>

#include <common/serialization/binary_archive.hpp>

#include "animation_graph_dataset.hpp"
#include "runtime_graph_node.hpp"

#include <vector>

namespace aln
{

/// @brief The definition of an animation graph represents all of its nodes' settings in a contiguous array.
/// It is created from editor only nodes via a compile step
/// @todo Holds info on required memory size for node instances and a vector of nodes offsets
class AnimationGraphDefinition : public IAsset
{
    ALN_REGISTER_ASSET_TYPE("agdf");

    friend class AnimationGraphCompilationContext;
    friend class AnimationGraphDefinitionLoader;
    friend class RuntimeGraphInstance;

  private:
    /// @todo Graph definitions can be associated with different anim datasets
    AssetHandle<AnimationGraphDataset> m_pDataset;
    std::vector<RuntimeGraphNode::Settings*> m_nodeSettings;

  public:
    size_t GetNumNodes() const { return m_nodeSettings.size(); }

    /// @brief Serialize the definition in a binary archive.
    /// @todo For now the serialization handles the nodes types as well as their data.
    // It might be cool to split the two, so that the "serialization" process remains concerned in data only
    // and only fills the definition's pre-allocated members
    void Serialize(BinaryMemoryArchive& archive) const
    {
        archive << GetNumNodes();

        // Save the settings types information so that we can instanciate the settings array
        std::vector<std::type_index> nodeSettingsTypeIndices;
        nodeSettingsTypeIndices.reserve(GetNumNodes());
        for (auto& pSettings : m_nodeSettings)
        {
            nodeSettingsTypeIndices.push_back(pSettings->GetType()->m_typeIndex);
        }
        archive << nodeSettingsTypeIndices;

        // Save the settings data
        for (auto& pSettings : m_nodeSettings)
        {
            pSettings->Serialize(archive);
        }
    }

    void Deserialize(BinaryMemoryArchive& archive)
    {
        size_t nodeCount;
        archive >> nodeCount;

        std::vector<std::type_index> nodeSettingsTypeIndices;
        archive >> nodeSettingsTypeIndices;

        // Calculate the memory requirements for the settings array
        // 1 - First loop to get the maximum required alignement
        size_t requiredMemoryAlignment = 0;
        size_t requiredMemorySize = 0;

        std::vector<const reflect::TypeDescriptor*> nodeSettingsTypeDescriptors;
        std::vector<size_t> nodeSettingsPaddings;

        nodeSettingsTypeDescriptors.reserve(nodeCount);
        for (auto& typeIndex : nodeSettingsTypeIndices)
        {
            const auto& pTypeDescriptor = reflect::GetType(typeIndex);
            requiredMemoryAlignment = std::max(requiredMemoryAlignment, pTypeDescriptor->alignment);
            nodeSettingsTypeDescriptors.push_back(pTypeDescriptor);
        }

        // 2 - Second one to calculate the total size + paddings
        /// @todo: How can we merge the first two loops ?
        nodeSettingsPaddings.reserve(nodeCount);
        for (auto i = 0; i < nodeCount; ++i)
        {
            auto nodeSettingsSize = nodeSettingsTypeDescriptors[i]->size;
            size_t requiredPadding = (requiredMemoryAlignment - (requiredMemorySize % requiredMemoryAlignment)) % requiredMemoryAlignment;
            requiredMemorySize += nodeSettingsSize + requiredPadding;
            nodeSettingsPaddings.push_back(requiredPadding);
        }

        // 3 - Allocate the memory for the actual settings data
        // /!\ The memory is not handled by the class but by the loader !
        auto pSettingsMemory = (std::byte*) aln::Allocate(requiredMemorySize, requiredMemoryAlignment);

        // 4 - Placement new each settings type in contiguous memory
        // and add a ptr to the definition's collection
        m_nodeSettings.reserve(nodeCount);
        for (auto i = 0; i < nodeCount; ++i)
        {
            pSettingsMemory += nodeSettingsPaddings[i];
            m_nodeSettings.push_back(nodeSettingsTypeDescriptors[i]->CreateTypeInPlace<RuntimeGraphNode::Settings>(pSettingsMemory));
            pSettingsMemory += nodeSettingsTypeDescriptors[i]->size;
        }

        // Deserialize the data into the nodes settings
        for (auto& pSettings : m_nodeSettings)
        {
            pSettings->Deserialize(archive);
        }
    }
};
} // namespace aln