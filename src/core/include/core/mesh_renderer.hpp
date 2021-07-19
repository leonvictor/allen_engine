#pragma once

#include <memory>
#include <vector>
#include <vulkan/vulkan.hpp>

#include <graphics/resources/buffer.hpp>
#include <graphics/resources/image.hpp>
#include <graphics/ubo.hpp>

#include <entities/spatial_component.hpp>

#include "material.hpp"
#include "mesh.hpp"

namespace aln
{
namespace vkg
{
class Device;
}

/// @brief The MeshRenderer component holds a mesh, its material, and the vulkan objects
// representing them on the GPU.
class MeshRenderer : public entities::SpatialComponent
{
    friend class GraphicsSystem;

  private:
    std::shared_ptr<vkg::Device> m_pDevice;

    Mesh m_mesh;
    Material m_material;

    vk::UniqueDescriptorSet m_vkDescriptorSet;
    vkg::resources::Buffer m_vertexBuffer;
    vkg::resources::Buffer m_indexBuffer;
    vkg::resources::Buffer m_uniformBuffer;
    vkg::resources::Buffer m_materialBuffer;
    vkg::resources::Image m_materialTexture;

    // -------------------------------------------------
    // Vulkan resources creation
    // -------------------------------------------------
    void CreateDataBuffers();
    void CreateMaterialTexture();
    void CreateUniformBuffer();
    void CreateDescriptorSet();

  public:
    /// @brief Construct a MeshRenderer component.
    /// @param pDevice: pointer to the rendering device.
    /// @param path: path to the model file.
    MeshRenderer(std::shared_ptr<vkg::Device> pDevice, std::string modelPath, std::string texturePath);

    /// @brief Returns the vulkan bindings representing a scene object.
    static std::vector<vk::DescriptorSetLayoutBinding> GetDescriptorSetLayoutBindings();
    vk::DescriptorSet& GetDescriptorSet();

    void UpdateUniformBuffers(vkg::UniformBufferObject& ubo);

    // -------------------------------------------------
    // Components Methods
    // -------------------------------------------------

    void Initialize() override;
    void Shutdown() override;
    bool Load() override;
    void Unload() override;
};
} // namespace aln