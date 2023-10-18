#include "descriptor_allocator.hpp"

#include <algorithm>
#include <stdexcept>

namespace aln
{

/// @brief Create a pool
vk::UniqueDescriptorPool CreatePool(vk::Device* pDevice, const DescriptorAllocator::PoolSizes& poolSizes, uint32_t count, vk::DescriptorPoolCreateFlags flags)
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

    auto [result, pDescriptorPool] = pDevice->createDescriptorPoolUnique(poolInfo);

    return std::move(pDescriptorPool);
}

void DescriptorAllocator::ResetPools()
{
    for (auto& p : m_usedPools)
    {
        m_pRenderEngine->resetDescriptorPool(p.get());
    }

    m_freePools = std::move(m_usedPools);
    m_usedPools.clear();

    m_currentPool = nullptr;
}

vk::UniqueDescriptorSet DescriptorAllocator::Allocate(const vk::DescriptorSetLayout* pLayout)
{
    assert(m_pRenderEngine != nullptr);

    if (m_currentPool == nullptr)
    {
        // Grab a pool and mark it for cleanup
        auto pool = GrabPool();
        m_usedPools.push_back(std::move(pool));
        m_currentPool = &m_usedPools.back();
    }

    vk::DescriptorSetAllocateInfo allocInfo = {
        .descriptorPool = m_currentPool->get(),
        .descriptorSetCount = 1,
        .pSetLayouts = pLayout,
    };

    bool needReallocate = false;
    auto result = m_pRenderEngine->allocateDescriptorSetsUnique(allocInfo);
    if (result.result == vk::Result::eSuccess)
    {
        return std::move(result.value[0]);
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
        m_currentPool = &m_usedPools.back();

        result = m_pRenderEngine->allocateDescriptorSetsUnique(allocInfo);
        if (result.result == vk::Result::eSuccess)
        {
            return std::move(result.value[0]);
        }
        else
        {
            // if it still fails then we have big issues
            assert(false); // Unrecoverable error during descriptor set allocation
        }
    }

    assert(false);
    return vk::UniqueDescriptorSet();
}

void DescriptorAllocator::Init(vk::Device* newDevice)
{
    m_pRenderEngine = newDevice;
}

void DescriptorAllocator::Cleanup()
{
    // delete every pool held
    for (auto& p : m_freePools)
    {
        p.reset();
    }
    m_freePools.clear();

    for (auto& p : m_usedPools)
    {
        p.reset();
    }
    m_usedPools.clear();
}

vk::UniqueDescriptorPool DescriptorAllocator::GrabPool()
{
    if (m_freePools.size() > 0)
    {
        auto pool = std::move(m_freePools.back());
        m_freePools.pop_back();
        return std::move(pool);
    }
    else
    {
        // Default: a new pool can hold 1000 descriptors. This is arbitrary.
        return std::move(CreatePool(m_pRenderEngine, descriptorSizes, 1000, vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet));
    }
}

vk::DescriptorPool& DescriptorAllocator::GetActivePool()
{
    if (m_currentPool == nullptr)
    {
        // Grab a pool and mark it for cleanup
        auto pool = GrabPool();
        m_usedPools.push_back(std::move(pool));
        m_currentPool = &m_usedPools.back();
    }

    return m_currentPool->get();
}
} // namespace aln