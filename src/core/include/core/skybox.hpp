#pragma once

#include <entities/component.hpp>
#include <graphics/device.hpp>
#include <graphics/resources/buffer.hpp>
#include <graphics/resources/image.hpp>
#include <graphics/ubo.hpp>

#include <memory>
#include <vulkan/vulkan.hpp>

#include <graphics/draw_mesh.hpp>
// TODO: Update to new syntax

// TODO: Where should skyboxes live ?
//  - They're unique
//  - They're static (for now at least)
// Unity has them as an attribute of the camera

namespace aln
{

class Skybox : public entities::IComponent
{
    friend class GraphicsSystem;

  private:
    std::shared_ptr<vkg::Device> m_pDevice;

    vkg::DrawMesh m_mesh;

    std::string m_cubeMapFolder;
    vkg::resources::Image m_cubeMap;

    vkg::resources::Buffer m_uniformBuffer;

    vk::UniqueDescriptorSet m_vkDescriptorSet;

  public:
    // TODO: TMP
    bool bound = false;

    /// @brief Construct a Skybox component.
    /// @param pDevice: pointer to the rendering device.
    /// @param modelPath: path to the model file
    /// @param texturePath: path to the texture folder
    ///
    /// @todo Model is a simple box so we shouldn't have to load a .obj
    /// -> primitives
    /// @todo How do we handle cubemaps nicely ?
    Skybox(std::shared_ptr<vkg::Device> pDevice, std::string modelPath, std::string texturePath);

    void CreateCubeMap();
    void CreateDescriptorSet();

    void CreateUniformBuffer();

    void UpdateUniformBuffer(vkg::UniformBufferObject& ubo);

    /// @brief Returns the vulkan bindings representing a skybox.
    static std::vector<vk::DescriptorSetLayoutBinding> GetDescriptorSetLayoutBindings();

    vk::DescriptorSet& GetDescriptorSet();

    // -------------------------------------------------
    // Components Methods
    // -------------------------------------------------

    void Initialize() override;
    void Shutdown() override;
    bool Load() override;
    void Unload() override;

    std::string GetComponentTypeName() override
    {
        return "Skybox";
    }
};
} // namespace aln