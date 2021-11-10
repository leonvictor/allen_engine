#pragma once

#include <memory>
#include <string>
#include <vector>

#include <vulkan/vulkan.hpp>

#include <graphics/resources/buffer.hpp>
#include <graphics/resources/image.hpp>
#include <graphics/ubo.hpp>

#include <entities/spatial_component.hpp>

#include "component_factory.hpp"
#include "material.hpp"
#include "mesh.hpp"

#include <assets/handle.hpp>

namespace aln
{
namespace vkg
{
class Device;
struct UniformBufferObject;
} // namespace vkg

/// @brief The MeshRenderer component holds a mesh, its material, and the vulkan objects
// representing them on the GPU.
class MeshRenderer : public entities::SpatialComponent
{
    ALN_REGISTER_TYPE();

    friend class GraphicsSystem;

  private:
    std::shared_ptr<vkg::Device> m_pDevice;
    /// @todo Find a good way to expose the asset manager.
    std::shared_ptr<AssetManager> m_pAssetManager = nullptr;

    AssetHandle<Mesh> m_pMesh;
    Material m_material;

    vk::UniqueDescriptorSet m_vkDescriptorSet;
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
    // TODO: Should this be public ?
    void Construct(const entities::ComponentCreationContext& ctx) override
    {
        m_pAssetManager = ctx.pAssetManager;
        m_pMesh = ctx.pAssetManager->Get<Mesh>(ctx.defaultModelPath);
        m_material.m_texturePath = ctx.defaultTexturePath;
        m_pDevice = ctx.graphicsDevice;
    }

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