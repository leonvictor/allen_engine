#pragma once

#include <assets/asset.hpp>
#include <assets/handle.hpp>

#include <common/serialization/binary_archive.hpp>
#include <reflection/type_info.hpp>

#include "animation_graph_dataset.hpp"
#include "runtime_graph_node.hpp"

#include <typeinfo>
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
        // TODO:

        // Save the settings types information so that we can instanciate the settings array
        // reflect::TypeCollectionDescriptor typeCollectionDesc;
        // for (auto& pSettings : m_nodeSettings)
        // {
        //     auto typeInfo = *(pSettings->GetTypeInfo());
        //     typeCollectionDesc.m_descriptors.push_back(typeInfo);
        // }
        // archive << typeCollectionDesc;

        // // Save the settings data
        // for (auto& pSettings : m_nodeSettings)
        // {
        //     pSettings->Serialize(archive);
        // }
    }

    void Deserialize(BinaryMemoryArchive& archive)
    {
        // reflect::TypeCollectionDescriptor typeCollectionDesc;
        // archive >> typeCollectionDesc;

        // typeCollectionDesc.InstanciateFixedSizeCollection(m_nodeSettings);

        // // Deserialize the data into the nodes settings
        // for (auto& pSettings : m_nodeSettings)
        // {
        //     pSettings->Deserialize(archive);
        // }
    }
};
} // namespace aln