#pragma once

#include <memory>
#include <string>
#include <vector>

#include <vulkan/vulkan.hpp>

#include <graphics/resources/buffer.hpp>
#include <graphics/resources/image.hpp>
#include <graphics/ubo.hpp>

#include <entities/spatial_component.hpp>

#include "../component_factory.hpp"
#include "../material.hpp"
#include "../mesh.hpp"
#include "../texture.hpp"

#include <assets/handle.hpp>
#include <assets/type_descriptors/handles.hpp>

namespace aln
{
namespace vkg
{
class Device;
struct UniformBufferObject;
} // namespace vkg

/// @brief The MeshComponent component holds a mesh, its material, and the vulkan objects
// representing them on the GPU.
class MeshComponent : public entities::SpatialComponent
{
    friend class GraphicsSystem;

  private:
    std::shared_ptr<vkg::Device> m_pDevice;
    /// @todo Find a good way to expose the asset manager.
    std::shared_ptr<AssetManager> m_pAssetManager = nullptr;

    AssetHandle<Mesh> m_pMesh;
    AssetHandle<Material> m_pMaterial;

    vk::UniqueDescriptorSet m_vkDescriptorSet;
    vkg::resources::Buffer m_uniformBuffer;

    // -------------------------------------------------
    // Vulkan resources creation
    // -------------------------------------------------
    void CreateDataBuffers();
    void CreateUniformBuffer();
    void CreateDescriptorSet();

  public:
    // TODO: Should this be public ?
    void Construct(const entities::ComponentCreationContext& ctx) override
    {
        m_pAssetManager = ctx.pAssetManager;
        m_pMesh = ctx.pAssetManager->Get<Mesh>(ctx.defaultModelPath);
        m_pMaterial = ctx.pAssetManager->Get<Material>("DefaultMaterial");
        m_pMaterial->SetAlbedoMap(m_pAssetManager->Get<Texture>(ctx.defaultTexturePath));
        m_pDevice = ctx.graphicsDevice;
    }

    /// @brief Returns the vulkan bindings representing a scene object.
    static std::vector<vk::DescriptorSetLayoutBinding> GetDescriptorSetLayoutBindings();
    vk::DescriptorSet& GetDescriptorSet();

    void UpdateUniformBuffers(vkg::UniformBufferObject& ubo);

  protected:
    MeshComponent() = default;
    // -------------------------------------------------
    // Components Methods
    // -------------------------------------------------

    virtual void Initialize() override;
    virtual void Shutdown() override;
    virtual bool Load() override;
    virtual void Unload() override;
};
} // namespace aln