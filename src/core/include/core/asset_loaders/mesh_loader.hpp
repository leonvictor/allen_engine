#pragma once

#include <assets/loader.hpp>

#include <graphics/resources/buffer.hpp>

namespace aln
{
class RenderEngine;

class MeshLoader : public IAssetLoader
{

  private:
    RenderEngine* m_pRenderEngine;

    GPUBuffer m_vertexStagingBuffer;
    GPUBuffer m_indexStagingBuffer;

    static constexpr uint32_t STAGING_BUFFER_SIZE = 64 * 1000 * 1000 * 8;

  public:
    MeshLoader(RenderEngine* pRenderEngine) : m_pRenderEngine(pRenderEngine) {}

    MeshLoader(const MeshLoader&) = delete;
    MeshLoader(MeshLoader&&) = delete;
    MeshLoader& operator=(const MeshLoader&) = delete;

    bool Load(AssetRequestContext& ctx, AssetRecord* pRecord, BinaryMemoryArchive& archive) override;
    void Unload(AssetRecord* pRecord) override;
    void InstallDependencies(AssetRecord* pAssetRecord, const Vector<IAssetHandle>& dependencies) override;
};
} // namespace aln