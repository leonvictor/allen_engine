#pragma once

#include <assets/asset.hpp>
#include <assets/handle.hpp>
#include <common/containers/vector.hpp>
#include <common/vertex.hpp>
#include <graphics/resources/buffer.hpp>

#include "material.hpp"

namespace aln
{

// TODO: Complete gltf format
// In glTF, meshes are defined as arrays of primitives. Primitives correspond to the data required for GPU draw calls. Primitives specify one or more attributes, corresponding to the vertex attributes used in the draw calls.
struct PrimitiveComponent
{
    uint32_t firstIndex;
    uint32_t indexCount;
    uint32_t materialIndex;
};

/// @brief Mesh asset
class Mesh : public IAsset
{
    // TODO: Mesh are virtual and can't be used as is. How do we reflect that in the asset system ?
    ALN_REGISTER_ASSET_TYPE("mesh");

    friend class MeshLoader;
    friend class GraphicsSystem;

  private:
    Vector<std::byte> m_vertices;
    Vector<uint32_t> m_indices;

    Vector<PrimitiveComponent> m_primitives;

    AssetHandle<Material> m_pMaterial;

    // Rendering resources
    resources::Buffer m_vertexBuffer;
    resources::Buffer m_indexBuffer;
    vk::UniqueDescriptorSet m_descriptorSet;

  public:
    const AssetHandle<Material>& GetMaterial() const { return m_pMaterial; }
    const resources::Buffer& GetVertexBuffer() const { return m_vertexBuffer; }
    const resources::Buffer& GetIndexBuffer() const { return m_indexBuffer; }
    uint32_t GetIndicesCount() const { return m_indices.size(); }
    const vk::DescriptorSet& GetDescriptorSet() const { return m_descriptorSet.get(); }

    static Vector<vk::DescriptorSetLayoutBinding> GetDescriptorSetLayoutBindings()
    {
        Vector<vk::DescriptorSetLayoutBinding> bindings = {
            {
                // Sampler
                .binding = 0,
                .descriptorType = vk::DescriptorType::eCombinedImageSampler,
                .descriptorCount = 1,
                .stageFlags = vk::ShaderStageFlagBits::eFragment,
            },
            {
                // Material
                .binding = 1,
                .descriptorType = vk::DescriptorType::eUniformBuffer,
                .descriptorCount = 1,
                .stageFlags = vk::ShaderStageFlagBits::eFragment,
            },
        };

        return bindings;
    }
};
} // namespace aln