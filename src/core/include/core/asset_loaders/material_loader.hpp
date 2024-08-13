#pragma once

#include <assets/loader.hpp>

namespace aln
{
class RenderEngine;

class MaterialLoader : public IAssetLoader
{
  private:
    RenderEngine* m_pRenderEngine;

  public:
    MaterialLoader(RenderEngine* pRenderEngine) : m_pRenderEngine(pRenderEngine) {}

    bool Load(AssetRequestContext& ctx, AssetRecord* pRecord, BinaryMemoryArchive& archive) override;
    void Unload(AssetRecord* pRecord) override;
    void InstallDependencies(AssetRecord* pAssetRecord, const Vector<IAssetHandle>& dependencies) override;
};

} // namespace aln