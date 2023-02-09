#pragma once

#include <assets/loader.hpp>

#include <anim/graph/animation_graph_dataset.hpp>
#include <anim/graph/graph_definition.hpp>

#include <memory>
#include <vector>

namespace aln
{

class AnimationGraphDefinitionLoader : public IAssetLoader
{
  public:
    bool Load(AssetRecord* pRecord, BinaryMemoryArchive& archive) override
    {
        assert(pRecord->IsUnloaded());
        assert(pRecord->GetAssetTypeID() == AnimationGraphDefinition::GetStaticAssetTypeID());

        AnimationGraphDefinition* pDefinition = aln::New<AnimationGraphDefinition>();

        // TODO: Settings memory is handled in the loader, so maybe its creation should be here as well ?
        pDefinition->Deserialize(archive);

        pRecord->SetAsset(pDefinition);
        return true;
    }

    virtual void Unload(AssetRecord* pRecord) override
    {
        auto pGraphDefinition = pRecord->GetAsset<AnimationGraphDefinition>();

        void* pMemory = pGraphDefinition->m_nodeSettings[0];
        for (auto& pSettings : pGraphDefinition->m_nodeSettings)
        {
            using T = RuntimeGraphNode::Settings;
            pSettings->~T();
        }
        aln::Free(pMemory);
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