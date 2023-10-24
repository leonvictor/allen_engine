#pragma once

#include "texture.hpp"

#include <assets/asset.hpp>
#include <assets/handle.hpp>
#include <common/maths/vec3.hpp>
#include <entities/component.hpp>
#include <graphics/resources/buffer.hpp>

#include <assert.h>

namespace aln
{
class Material : public IAsset
{
    ALN_REGISTER_ASSET_TYPE("mtrl");

    friend class MaterialLoader;
    friend class MeshComponent;

  private:
    // TODO: Other maps
    // TODO: Shaders...

    GPUBuffer m_buffer;
    AssetHandle<Texture> m_albedoMap;

  public:
    inline const AssetHandle<Texture>& GetAlbedoMap() const { return m_albedoMap; }
    inline const GPUBuffer& GetBuffer() const { return m_buffer; }
};

#pragma warning(disable:4324)
// TODO: This is never used. Refactor the material system
struct MaterialBufferObject
{
    alignas(16) Vec3 ambient = Vec3(1.0f, 0.5f, 0.31f);
    alignas(16) Vec3 diffuse = Vec3(1.0f, 0.5f, 0.31f);
    alignas(16) Vec3 specular = Vec3(0.5f, 0.5f, 0.5f);
    alignas(4) float shininess = 8.0f;
};
#pragma warning(default: 4324)

} // namespace aln