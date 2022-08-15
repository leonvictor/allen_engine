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

  protected:
    std::shared_ptr<vkg::Device> m_pDevice;
    /// @todo Find a good way to expose the asset manager.
    AssetManager* m_pAssetManager = nullptr;

    AssetHandle<Material> m_pMaterial;

    vk::UniqueDescriptorSet m_vkDescriptorSet;
    vkg::resources::Buffer m_uniformBuffer;

    // -------------------------------------------------
    // Vulkan resources creation
    // -------------------------------------------------
    void CreateDataBuffers();
    void CreateUniformBuffer();
    virtual void CreateDescriptorSet() = 0;

  public:
    // TODO: Should this be public ?
    virtual void Construct(const entities::ComponentCreationContext& ctx) override
    {
        m_pAssetManager = ctx.pAssetManager;
        m_pMaterial = AssetHandle<Material>(ctx.defaultMaterialPath);
        m_pDevice = ctx.graphicsDevice;
    }

    virtual vk::DescriptorSet& GetDescriptorSet();

    void UpdateUniformBuffers(vkg::UniformBufferObject& ubo);

  protected:
    MeshComponent() = default;
    // -------------------------------------------------
    // Components Methods
    // -------------------------------------------------

    virtual void Initialize() override;
    virtual void Shutdown() override;
    virtual void Load() override;
    virtual void Unload() override;

    virtual bool UpdateLoadingStatus() override = 0;
};
} // namespace aln