#include "descriptor_allocator.hpp"

#include <algorithm>
#include <stdexcept>

namespace aln::vkg
{

/// @brief Create a pool
vk::UniqueDescriptorPool CreatePool(vk::Device* pDevice, const DescriptorAllocator::PoolSizes& poolSizes, int count, vk::DescriptorPoolCreateFlags flags)
{
    Vector<vk::DescriptorPoolSize> sizes;
    sizes.reserve(poolSizes.sizes.size());
    for (auto sz : poolSizes.sizes)
    {
        sizes.push_back({sz.first, uint32_t(sz.second * count)});
    }

    vk::DescriptorPoolCreateInfo pool_info;
    pool_info.flags = flags;
    pool_info.maxSets = count;
    pool_info.poolSizeCount = (uint32_t) sizes.size();
    pool_info.pPoolSizes = sizes.data();

    vk::UniqueDescriptorPool descriptorPool = pDevice->createDescriptorPoolUnique(pool_info);

    return std::move(descriptorPool);
}

void DescriptorAllocator::ResetPools()
{
    for (auto& p : m_usedPools)
    {
        m_pDevice->resetDescriptorPool(p.get());
    }

    m_freePools = std::move(m_usedPools);
    m_usedPools.clear();

    m_currentPool = nullptr;
}

vk::UniqueDescriptorSet DescriptorAllocator::Allocate(const vk::DescriptorSetLayout* pLayout)
{
    assert(m_pDevice != nullptr);

    if (m_currentPool == nullptr)
    {
        // Grab a pool and mark it for cleanup
        auto pool = GrabPool();
        m_usedPools.push_back(std::move(pool));
        m_currentPool = &m_usedPools.back();
    }

    vk::DescriptorSetAllocateInfo allocInfo;
    allocInfo.descriptorPool = m_currentPool->get();
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = pLayout;

    bool needReallocate = false;

    try
    {
        auto sets = m_pDevice->allocateDescriptorSetsUnique(allocInfo);
        return std::move(sets[0]);
    }
    catch (const vk::FragmentedPoolError& e)
    {
        needReallocate = true;
    }
    catch (const vk::OutOfPoolMemoryError& e)
    {
        needReallocate = true;
    }
    catch (const std::exception& e)
    {
        //unrecoverable error
        throw std::runtime_error("Unrecoverable error during descriptor set allocation.");
    }

    if (needReallocate)
    {
        //allocate a new pool and retry
        auto pool = GrabPool();
        m_usedPools.push_back(std::move(pool));
        m_currentPool = &m_usedPools.back();

        try
        {
            auto sets = m_pDevice->allocateDescriptorSetsUnique(allocInfo);
            return std::move(sets[0]);
        }
        catch (std::exception const& e)
        {
            //if it still fails then we have big issues
            throw std::runtime_error("Unrecoverable error during descriptor set allocation.");
        }
    }

    assert(false);
    return vk::UniqueDescriptorSet();
}

void DescriptorAllocator::Init(vk::Device* newDevice)
{
    m_pDevice = newDevice;
}

void DescriptorAllocator::Cleanup()
{
    //delete every pool held
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
        return std::move(CreatePool(m_pDevice, descriptorSizes, 1000, vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet));
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

} // namespace aln::vkg