#pragma once

#include <memory>
#include <string>
#include <vector>

#include <assets/asset.hpp>
#include <common/vertex.hpp>
#include <graphics/device.hpp>
#include <graphics/resources/buffer.hpp>

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
    friend class MeshLoader;

  private:
    std::vector<Vertex> m_vertices;
    std::vector<uint32_t> m_indices;

    std::vector<PrimitiveComponent> m_primitives;

    vkg::resources::Buffer m_vertexBuffer;
    vkg::resources::Buffer m_indexBuffer;

    /// @brief Create and fill the vulkan buffers to back the mesh.
    void CreateGraphicResources(const std::shared_ptr<vkg::Device>&);

    /// @brief Reset the vulkan buffers backing the mesh on GPU.
    void FreeGraphicResources();

  public:
    Mesh(AssetGUID& guid) : IAsset(guid) {}
    void Bind(vk::CommandBuffer& cb, vk::DeviceSize offset);
    void RevertNormals();
};
} // namespace aln