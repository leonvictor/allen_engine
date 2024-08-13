#pragma once

#include <assets/loader.hpp>

namespace aln
{

class TextureLoader : public IAssetLoader
{
  private:
    RenderEngine* m_pRenderEngine;

  public:
    TextureLoader(RenderEngine* pRenderEngine) : m_pRenderEngine(pRenderEngine) {}

    bool Load(AssetRequestContext& ctx, AssetRecord* pRecord, BinaryMemoryArchive& archive) override;
    void Unload(AssetRecord* pRecord) override;
};

} // namespace aln