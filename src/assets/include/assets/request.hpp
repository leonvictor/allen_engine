#pragma once

#include "handle.hpp"
#include "loader.hpp"
#include "record.hpp"

#include <common/uuid.hpp>
#include <graphics/render_engine.hpp>

#include <vulkan/vulkan.hpp>

namespace aln
{
class AssetService;

struct AssetRequest
{
    enum class Type : uint8_t
    {
        Load,
        Unload,
        Invalid,
    };

    enum class State : uint8_t
    {
        Invalid,
        Pending,
        Loading,
        WaitingForDependencies,
        Installing,
        Complete,
        Unloading,
        Failed
    };

    UUID m_requesterEntityID = UUID::InvalidID;
    AssetRecord* m_pAssetRecord = nullptr;
    IAssetLoader* m_pLoader = nullptr;
    RenderEngine* m_pRenderDevice = nullptr;

    std::function<void(IAssetHandle&)> m_requestAssetLoad;
    std::function<void(IAssetHandle&)> m_requestAssetUnload;

    // Sync
    uint32_t m_threadIdx = 0; // Thread this request is being processed on

    vk::CommandBuffer* m_pTransferCommandBuffer = nullptr;
    vk::CommandBuffer* m_pGraphicsCommandBuffer = nullptr;
    vk::UniqueSemaphore m_pTransferQueueCommandsSemaphore; // Signaled when commands submitted to the transfer queue are done (i.e. vertex buffer cpu->gpu)
    vk::UniqueSemaphore m_pGraphicsQueueCommandsSemaphore; // Signaled when commands submitted to the graphics queue are done (i.e. mipmap generation)
    bool m_commandBuffersSubmitted = false;
    
    Type m_type = Type::Invalid;
    State m_status = State::Invalid;

    Vector<IAssetHandle> m_dependencies;

    bool IsValid() const { return m_type != Type::Invalid; }
    bool IsLoadingRequest() const { return m_type == Type::Load; }
    bool IsUnloadingRequest() const { return m_type == Type::Unload; }

    void Load();
    void WaitForDependencies();
    void Install();
    void Unload();

    bool IsComplete() { return m_status == State::Complete; }

    bool HasTouchedGPUTransferQueue() const { return (bool) m_pTransferQueueCommandsSemaphore; }
    bool HasTouchedGPUGraphicsQueue() const { return (bool) m_pGraphicsQueueCommandsSemaphore; }
    bool HasTouchedAnyGPUQueue() const { return HasTouchedGPUGraphicsQueue() || HasTouchedGPUTransferQueue(); }

    vk::CommandBuffer* GetTransferCommandBuffer()
    {
        assert(m_pRenderDevice != nullptr && m_pTransferCommandBuffer != nullptr);
        if (!m_pTransferQueueCommandsSemaphore)
        {
            static constexpr vk::SemaphoreTypeCreateInfo semaphoreTypeCreateInfo = {
                .semaphoreType = vk::SemaphoreType::eTimeline,
                .initialValue = 0,
            };

            static constexpr vk::SemaphoreCreateInfo semaphoreCreateInfo = {
                .pNext = &semaphoreTypeCreateInfo,
            };

            m_pTransferQueueCommandsSemaphore = m_pRenderDevice->GetVkDevice().createSemaphoreUnique(semaphoreCreateInfo).value;
        }

        return m_pTransferCommandBuffer;
    }

    vk::CommandBuffer* GetGraphicsCommandBuffer()
    {
        assert(m_pRenderDevice != nullptr && m_pGraphicsCommandBuffer != nullptr);
        if (!m_pGraphicsQueueCommandsSemaphore)
        {
            static constexpr vk::SemaphoreTypeCreateInfo semaphoreTypeCreateInfo = {
                .semaphoreType = vk::SemaphoreType::eTimeline,
                .initialValue = 0,
            };

            static constexpr vk::SemaphoreCreateInfo semaphoreCreateInfo = {
                .pNext = &semaphoreTypeCreateInfo,
            };

            m_pGraphicsQueueCommandsSemaphore = m_pRenderDevice->GetVkDevice().createSemaphoreUnique(semaphoreCreateInfo).value;
        }

        return m_pGraphicsCommandBuffer;
    }
};
} // namespace aln