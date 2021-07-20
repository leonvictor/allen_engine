#include "skybox.hpp"

namespace aln
{
Skybox::Skybox(std::shared_ptr<vkg::Device> pDevice, std::string modelPath, std::string texturePath)
    : m_pDevice(pDevice), m_mesh(modelPath), m_cubeMapFolder(texturePath) {}

void Skybox::CreateCubeMap()
{
    m_cubeMap = vkg::resources::Image::CubemapFromDirectory(m_pDevice, m_cubeMapFolder);
    // TODO: Viewtype is known as it's already a cubemap
    m_cubeMap.AddView(vk::ImageAspectFlagBits::eColor, vk::ImageViewType::eCube);
    m_cubeMap.AddSampler(vk::SamplerAddressMode::eClampToEdge);
}

// TODO: Maybe move the alloc/update to swapchain ? See mesh.cpp
void Skybox::CreateDescriptorSet()
{
    m_vkDescriptorSet = m_pDevice->AllocateDescriptorSet<Skybox>();
    m_pDevice->SetDebugUtilsObjectName(m_vkDescriptorSet.get(), "Skybox Descriptor Set");

    auto uboDescriptor = m_uniformBuffer.GetDescriptor();
    auto cubeMapDescriptor = m_cubeMap.GetDescriptor();

    std::vector<vk::WriteDescriptorSet> writeDescriptors = {
        {
            .dstSet = m_vkDescriptorSet.get(),
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eUniformBuffer,
            .pImageInfo = nullptr,
            .pBufferInfo = &uboDescriptor,
            .pTexelBufferView = nullptr,
        },
        {
            .dstSet = m_vkDescriptorSet.get(),
            .dstBinding = 1,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eCombinedImageSampler,
            .pImageInfo = &cubeMapDescriptor,
            .pBufferInfo = nullptr,
            .pTexelBufferView = nullptr,
        },
    };

    m_pDevice->GetVkDevice().updateDescriptorSets(writeDescriptors, nullptr);
}

void Skybox::CreateUniformBuffer()
{
    m_uniformBuffer = vkg::resources::Buffer(m_pDevice, sizeof(vkg::UniformBufferObject), vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
}

void Skybox::UpdateUniformBuffer(vkg::UniformBufferObject& ubo)
{
    m_uniformBuffer.Map(0, sizeof(ubo));
    m_uniformBuffer.Copy(&ubo, sizeof(ubo));
    m_uniformBuffer.Unmap();
}

std::vector<vk::DescriptorSetLayoutBinding> Skybox::GetDescriptorSetLayoutBindings()
{
    std::vector<vk::DescriptorSetLayoutBinding> bindings{
        {0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex},
        {1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}};

    return bindings;
}

vk::DescriptorSet& Skybox::GetDescriptorSet() { return m_vkDescriptorSet.get(); }

// -------------------------------------------------
// Components Methods
// -------------------------------------------------

void Skybox::Initialize()
{
    m_mesh.CreateGraphicResources(m_pDevice);
    CreateUniformBuffer();
    CreateCubeMap();
    CreateDescriptorSet();
}

void Skybox::Shutdown()
{
    m_vkDescriptorSet.reset();
    m_cubeMap = vkg::resources::Image();
    m_uniformBuffer = vkg::resources::Buffer();
    m_mesh.FreeGraphicResources();

    // TODO: Device should be in the context.
    m_pDevice.reset();
}

bool Skybox::Load()
{
    if (m_mesh.Load())
    {
        // m_mesh.RevertNormals();
        return true;
    }
    return false;
}

void Skybox::Unload()
{
    m_mesh.Unload();
}
} // namespace aln