#pragma once

#include <assets/loader.hpp>

#include <anim/graph/animation_graph_dataset.hpp>
#include <anim/graph/graph_definition.hpp>
#include <reflection/services/type_registry_service.hpp>

#include <memory>
#include <vector>

namespace aln
{

class AnimationGraphDefinitionLoader : public IAssetLoader
{
  private:
    const TypeRegistryService* m_pTypeRegistryService;

  public:
    AnimationGraphDefinitionLoader(const TypeRegistryService* pTypeRegistryService)
        : m_pTypeRegistryService(pTypeRegistryService) {}

    bool Load(AssetRecord* pRecord, BinaryMemoryArchive& archive) override
    {
        assert(pRecord->IsUnloaded());
        assert(pRecord->GetAssetTypeID() == AnimationGraphDefinition::GetStaticAssetTypeID());

        AnimationGraphDefinition* pDefinition = aln::New<AnimationGraphDefinition>();

        // TODO: Settings memory is handled by the loader, so maybe its creation should be here as well ?
        reflect::TypeCollectionDescriptor typeCollectionDesc;
        archive >> typeCollectionDesc;
        archive >> *pDefinition;

        typeCollectionDesc.InstanciateFixedSizeCollection(pDefinition->m_nodeSettings, *m_pTypeRegistryService);

        pRecord->SetAsset(pDefinition);
        return true;
    }

    virtual void Unload(AssetRecord* pRecord) override
    {
        auto pGraphDefinition = pRecord->GetAsset<AnimationGraphDefinition>();

        // TODO: Could be moved to type collection logics (something like DeleteFixedSizeCollection)
        void* pMemory = pGraphDefinition->m_nodeSettings[0];
        for (auto& pSettings : pGraphDefinition->m_nodeSettings)
        {
            using T = RuntimeGraphNode::Settings;
            pSettings->~T();
        }
        aln::Free(pMemory);

        pGraphDefinition->m_nodeSettings.clear();
        pGraphDefinition->m_nodeIndices.clear();
        pGraphDefinition->m_rootNodeIndex = InvalidIndex;
        pGraphDefinition->m_nodeOffsets.clear();
        pGraphDefinition->m_requiredMemoryAlignement = 0;
        pGraphDefinition->m_requiredMemorySize = 0;
    };
};

class AnimationGraphDatasetLoader : public IAssetLoader
{
  public:
    bool Load(AssetRecord* pRecord, BinaryMemoryArchive& archive) override
    {
        assert(pRecord->IsUnloaded());
        assert(pRecord->GetAssetTypeID() == AnimationGraphDataset::GetStaticAssetTypeID());

        AnimationGraphDataset* pDataset = aln::New<AnimationGraphDataset>();

        size_t clipCount;
        AssetID assetID;

        archive >> clipCount;

        pDataset->m_animationClips.reserve(clipCount);
        for (size_t i = 0; i < clipCount; ++i)
        {
            archive >> assetID;
            pDataset->m_animationClips.emplace_back(assetID);
        }

        pRecord->SetAsset(pDataset);
        return true;
    }

    void InstallDependencies(AssetRecord* pAssetRecord, const std::vector<IAssetHandle>& dependencies) override
    {
        auto pDataset = pAssetRecord->GetAsset<AnimationGraphDataset>();

        for (auto& clipHandle : pDataset->m_animationClips)
        {
            // TODO: This is a lot of loops
            assert(clipHandle.GetAssetID().IsValid());
            clipHandle.m_pAssetRecord = GetDependencyRecord(dependencies, clipHandle.GetAssetID());
        }
    }
};

} // namespace aln