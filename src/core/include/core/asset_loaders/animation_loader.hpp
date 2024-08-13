#pragma once

#include <assets/loader.hpp>

namespace aln
{
class RenderEngine;

class AnimationLoader : public IAssetLoader
{
  private:
    RenderEngine* m_pRenderEngine;

  public:
    AnimationLoader(RenderEngine* pRenderEngine) : m_pRenderEngine(pRenderEngine) {}

    bool Load(AssetRequestContext& ctx, AssetRecord* pRecord, BinaryMemoryArchive& archive) override;
    void InstallDependencies(AssetRecord* pAssetRecord, const Vector<IAssetHandle>& dependencies) override;
};
} // namespace aln