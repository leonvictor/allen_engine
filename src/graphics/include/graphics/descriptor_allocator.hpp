// Adapted from vkguide: https://github.com/vblanco20-1/vulkan-guide/blob/engine/extra-engine/
// TODO: Multithreaded version here: https://github.com/vblanco20-1/Vulkan-Descriptor-Allocator

// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <common/containers/vector.hpp>

#include <vulkan/vulkan.hpp>

namespace aln
{

class DescriptorAllocator
{
  public:
    struct PoolSizes
    {
        Vector<std::pair<vk::DescriptorType, float>> sizes =
            {
                {vk::DescriptorType::eSampler, 0.5f},
                {vk::DescriptorType::eCombinedImageSampler, 4.f},
                {vk::DescriptorType::eSampledImage, 4.f},
                {vk::DescriptorType::eStorageImage, 1.f},
                {vk::DescriptorType::eUniformTexelBuffer, 1.f},
                {vk::DescriptorType::eStorageTexelBuffer, 1.f},
                {vk::DescriptorType::eUniformBuffer, 2.f},
                {vk::DescriptorType::eStorageBuffer, 2.f},
                {vk::DescriptorType::eUniformBufferDynamic, 1.f},
                {vk::DescriptorType::eStorageBufferDynamic, 1.f},
                {vk::DescriptorType::eInputAttachment, 0.5f}};
    };

    /// @brief: Resets all the dirty pools and ready them for reuse.
    void ResetPools();
    vk::DescriptorSet Allocate(const vk::DescriptorSetLayout* pLayout);

    void Initialize(vk::Device* pDevice);
    void Shutdown();

    vk::DescriptorPool& GetActivePool();

  private:
    vk::Device* m_pLogicalDevice = nullptr;
    PoolSizes descriptorSizes;

    /// Pointer to the current active pool.
    vk::DescriptorPool* m_pCurrentPool = nullptr;

    /// Used pools for cleanup.
    Vector<vk::DescriptorPool> m_usedPools;
    /// Free pools available to reuse.
    Vector<vk::DescriptorPool> m_freePools;

    /// @brief: Grab a free pool if one is available, otherwise create a new one and return it.
    vk::DescriptorPool GrabPool();
};
} // namespace aln
