#include "descriptor_allocator.hpp"

#include <algorithm>
#include <stdexcept>

namespace aln
{

/// @brief Create a pool
vk::DescriptorPool CreatePool(vk::Device* pDevice, const DescriptorAllocator::PoolSizes& poolSizes, uint32_t count, vk::DescriptorPoolCreateFlags flags)
{
    Vector<vk::DescriptorPoolSize> sizes;
    sizes.reserve(poolSizes.sizes.size());
    for (auto sz : poolSizes.sizes)
    {
        sizes.push_back({sz.first, uint32_t(sz.second * count)});
    }

    vk::DescriptorPoolCreateInfo poolInfo = {
        .flags = flags,
        .maxSets = count,
        .poolSizeCount = (uint32_t) sizes.size(),
        .pPoolSizes = sizes.data(),
    };

    auto descriptorPool = pDevice->createDescriptorPool(poolInfo).value;

    return descriptorPool;
}

void DescriptorAllocator::ResetPools()
{
    for (auto& p : m_usedPools)
    {
        m_pLogicalDevice->resetDescriptorPool(p);
    }

    m_freePools = std::move(m_usedPools);
    m_usedPools.clear();

    m_pCurrentPool = nullptr;
}

vk::DescriptorSet DescriptorAllocator::Allocate(const vk::DescriptorSetLayout* pLayout)
{
    assert(m_pLogicalDevice != nullptr);

    if (m_pCurrentPool == nullptr)
    {
        // Grab a pool and mark it for cleanup
        auto pool = GrabPool();
        m_usedPools.push_back(std::move(pool));
        m_pCurrentPool = &m_usedPools.back();
    }

    vk::DescriptorSetAllocateInfo allocInfo = {
        .descriptorPool = *m_pCurrentPool,
        .descriptorSetCount = 1,
        .pSetLayouts = pLayout,
    };

    bool needReallocate = false;
    auto result = m_pLogicalDevice->allocateDescriptorSets(allocInfo);
    if (result.result == vk::Result::eSuccess)
    {
        return result.value[0];
    }
    else if (result.result == vk::Result::eErrorFragmentedPool || result.result == vk::Result::eErrorOutOfPoolMemory)
    {
        needReallocate = true;
    }
    else
    {
        assert(false); // Unrecoverable error during descriptor set allocation
    }

    if (needReallocate)
    {
        // Allocate a new pool and retry
        auto pool = GrabPool();
        m_usedPools.push_back(std::move(pool));
        m_pCurrentPool = &m_usedPools.back();

        result = m_pLogicalDevice->allocateDescriptorSets(allocInfo);
        if (result.result == vk::Result::eSuccess)
        {
            return result.value[0];
        }
        else
        {
            // if it still fails then we have big issues
            assert(false); // Unrecoverable error during descriptor set allocation
        }
    }

    assert(false);
    return vk::DescriptorSet();
}

void DescriptorAllocator::Initialize(vk::Device* pLogicalDevice)
{
    m_pLogicalDevice = pLogicalDevice;
}

void DescriptorAllocator::Shutdown()
{
    // delete every pool held
    for (auto& freePool : m_freePools)
    {
        m_pLogicalDevice->destroyDescriptorPool(freePool);
    }
    m_freePools.clear();

    for (auto& freePool : m_usedPools)
    {
        m_pLogicalDevice->destroyDescriptorPool(freePool);
    }
    m_usedPools.clear();
}

vk::DescriptorPool DescriptorAllocator::GrabPool()
{
    if (m_freePools.size() > 0)
    {
        auto pool = std::move(m_freePools.back());
        m_freePools.pop_back();
        return pool;
    }
    else
    {
        // Default: a new pool can hold 1000 descriptors. This is arbitrary.
        return CreatePool(m_pLogicalDevice, descriptorSizes, 1000, vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet);
    }
}

vk::DescriptorPool& DescriptorAllocator::GetActivePool()
{
    if (m_pCurrentPool == nullptr)
    {
        // Grab a pool and mark it for cleanup
        auto pool = GrabPool();
        m_usedPools.push_back(std::move(pool));
        m_pCurrentPool = &m_usedPools.back();
    }

    return *m_pCurrentPool;
}
} // namespace aln