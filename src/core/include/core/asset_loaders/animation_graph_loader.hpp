#pragma once

#include <assets/loader.hpp>

namespace aln
{
class TypeRegistryService;

class AnimationGraphDefinitionLoader : public IAssetLoader
{
  private:
    const TypeRegistryService* m_pTypeRegistryService;

  public:
    AnimationGraphDefinitionLoader(const TypeRegistryService* pTypeRegistryService)
        : m_pTypeRegistryService(pTypeRegistryService) {}

    bool Load(AssetRequestContext& ctx, AssetRecord* pRecord, BinaryMemoryArchive& archive) override;
    void Unload(AssetRecord* pRecord) override;
};

class AnimationGraphDatasetLoader : public IAssetLoader
{
  public:
    bool Load(AssetRequestContext& ctx, AssetRecord* pRecord, BinaryMemoryArchive& archive) override;
    void InstallDependencies(AssetRecord* pAssetRecord, const Vector<IAssetHandle>& dependencies) override;
};

} // namespace aln