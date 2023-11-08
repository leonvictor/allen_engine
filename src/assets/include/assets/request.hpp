#pragma once

#include "handle.hpp"
#include "loader.hpp"
#include "record.hpp"
#include "request_context.hpp"

#include <common/uuid.hpp>
#include <graphics/render_engine.hpp>

#include <vulkan/vulkan.hpp>

namespace aln
{
class AssetService;

class AssetRequest
{
    friend class AssetService;

  private:
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

  private:
    UUID m_requesterEntityID = UUID::InvalidID;
    AssetRecord* m_pAssetRecord = nullptr;
    IAssetLoader* m_pLoader = nullptr;
    RenderEngine* m_pRenderDevice = nullptr;

    // Callbacks to queue dependency requests
    std::function<void(IAssetHandle&)> m_requestAssetLoad;
    std::function<void(IAssetHandle&)> m_requestAssetUnload;

    // Sync
    AssetRequestContext m_context;

    vk::Semaphore m_transferQueueCommandsSemaphore; // Signaled when commands submitted to the transfer queue are done (i.e. vertex buffer cpu->gpu)
    vk::Semaphore m_graphicsQueueCommandsSemaphore; // Signaled when commands submitted to the graphics queue are done (i.e. mipmap generation)
    bool m_commandBuffersSubmitted = false;

    Type m_type = Type::Invalid;
    State m_status = State::Invalid;

    Vector<IAssetHandle> m_dependencies;

  private:
    void Load();
    void WaitForDependencies();
    void Install();
    void Unload();

    void Shutdown()
    {
        m_pRenderDevice->GetVkDevice().destroySemaphore(m_transferQueueCommandsSemaphore);
        m_pRenderDevice->GetVkDevice().destroySemaphore(m_graphicsQueueCommandsSemaphore);
    }

  public:
    bool IsValid() const { return m_type != Type::Invalid; }
    bool IsLoadingRequest() const { return m_type == Type::Load; }
    bool IsUnloadingRequest() const { return m_type == Type::Unload; }
    bool IsComplete() { return m_status == State::Complete; }

    bool WereCommandsSubmitted() const { return m_commandBuffersSubmitted; }
    bool HasTouchedGPUTransferQueue() const { return (bool) m_transferQueueCommandsSemaphore; }
    bool HasTouchedGPUGraphicsQueue() const { return (bool) m_graphicsQueueCommandsSemaphore; }
    bool AreGraphicsCommandsInProgress() const { return m_graphicsQueueCommandsSemaphore && m_pRenderDevice->GetVkDevice().getSemaphoreCounterValue(m_graphicsQueueCommandsSemaphore).value == 0; }
    bool AreTransferCommandsInProgress() const { return m_transferQueueCommandsSemaphore && m_pRenderDevice->GetVkDevice().getSemaphoreCounterValue(m_transferQueueCommandsSemaphore).value == 0; }

    bool FinalizeTransferQueueCommands()
    {
        if (m_context.WasTransferSubmissionAccessed())
        {
            static constexpr vk::SemaphoreTypeCreateInfo semaphoreTypeCreateInfo = {
                .semaphoreType = vk::SemaphoreType::eTimeline,
                .initialValue = 0,
            };

            static constexpr vk::SemaphoreCreateInfo semaphoreCreateInfo = {
                .pNext = &semaphoreTypeCreateInfo,
            };

            m_transferQueueCommandsSemaphore = m_pRenderDevice->GetVkDevice().createSemaphore(semaphoreCreateInfo).value;
            m_pRenderDevice->SetDebugUtilsObjectName(m_transferQueueCommandsSemaphore, "Asset Request Transfer Queue Semaphore");

            m_context.GetTransferQueueSubmission()->SignalSemaphore(&m_transferQueueCommandsSemaphore, 1);
            return true;
        }

        return false;
    }

    bool FinalizeGraphicsQueueCommands()
    {
        if (m_context.WasGraphicsSubmissionAccessed())
        {
            static constexpr vk::SemaphoreTypeCreateInfo semaphoreTypeCreateInfo = {
                .semaphoreType = vk::SemaphoreType::eTimeline,
                .initialValue = 0,
            };

            static constexpr vk::SemaphoreCreateInfo semaphoreCreateInfo = {
                .pNext = &semaphoreTypeCreateInfo,
            };

            m_graphicsQueueCommandsSemaphore = m_pRenderDevice->GetVkDevice().createSemaphore(semaphoreCreateInfo).value;
            m_pRenderDevice->SetDebugUtilsObjectName(m_graphicsQueueCommandsSemaphore, "Asset Request Graphics Queue Semaphore");

            m_context.GetGraphicsQueueSubmission()->SignalSemaphore(&m_graphicsQueueCommandsSemaphore, 1);
            return true;
        }

        return false;
    }
};
} // namespace aln