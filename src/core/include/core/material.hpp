#pragma once

#include <assets/asset.hpp>
#include <assets/handle.hpp>
#include <entities/component.hpp>
#include <graphics/resources/buffer.hpp>

#include <assert.h>
#include <glm/gtc/vec1.hpp>
#include <glm/vec3.hpp>

#include "texture.hpp"

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
    AssetHandle<Texture> m_albedoMap;

    vkg::resources::Buffer m_buffer;

  public:
    Material(AssetID& guid) : IAsset(guid) {}

    void SetAlbedoMap(const AssetHandle<Texture>& pTex)
    {
        if (pTex == m_albedoMap)
            return;

        assert(IAsset::IsUnloaded());

        if (m_albedoMap)
            RemoveDependency(m_albedoMap->GetID());

        AddDependency<Texture>(pTex->GetID());
        m_albedoMap = pTex;
    }
};

// TODO: This is never used. Refactor the material system
struct MaterialBufferObject
{
    alignas(16) glm::vec3 ambient = glm::vec3(1.0f, 0.5f, 0.31f);
    alignas(16) glm::vec3 diffuse = glm::vec3(1.0f, 0.5f, 0.31);
    alignas(16) glm::vec3 specular = glm::vec3(0.5f, 0.5f, 0.5);
    alignas(4) glm::vec1 shininess = glm::vec1(8.0f);
};
} // namespace aln