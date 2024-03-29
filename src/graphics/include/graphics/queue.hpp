#pragma once

#include "command_buffer.hpp"

#include <common/containers/vector.hpp>

#include <vulkan/vulkan.hpp>

#include <optional>

namespace aln
{
// Wrapper around a VkSubmitInfo that automatically handles persistent CBs' events
class QueueSubmissionRequest
{
    friend class Queue;

  private:
    Vector<vk::CommandBufferSubmitInfo> m_commandBufferSubmitInfos;
    Vector<vk::SemaphoreSubmitInfo> m_signalSemaphoreSubmitInfos;
    Vector<vk::SemaphoreSubmitInfo> m_waitSemaphoreSubmitInfos;

  public:
    void ExecuteCommandBuffer(PersistentCommandBuffer<vk::Event>& persistentCB)
    {
        assert(persistentCB);

        persistentCB.m_pCommandBuffer->resetEvent(*persistentCB.m_pSyncPrimitive, {});
        persistentCB.m_pCommandBuffer->end();

        auto& cbSubmitInfo = m_commandBufferSubmitInfos.emplace_back();
        cbSubmitInfo.commandBuffer = *persistentCB.m_pCommandBuffer;
    }

    void ExecuteCommandBuffer(PersistentCommandBuffer<TimelineSemaphore>& persistentCB)
    {
        assert(persistentCB);

        persistentCB.m_pCommandBuffer->end();

        auto& cbSubmitInfo = m_commandBufferSubmitInfos.emplace_back();
        cbSubmitInfo.commandBuffer = *persistentCB.m_pCommandBuffer;

        auto& signalSubmitInfo = m_signalSemaphoreSubmitInfos.emplace_back();
        signalSubmitInfo.semaphore = persistentCB.m_pSyncPrimitive->m_semaphore;
        signalSubmitInfo.value = persistentCB.m_pSyncPrimitive->m_value;
    }

    void ExecuteCommandBuffer(TransientCommandBuffer& transientCB)
    {
        assert(transientCB);

        transientCB.m_pCommandBuffer->end();

        auto& cbSubmitInfo = m_commandBufferSubmitInfos.emplace_back();
        cbSubmitInfo.commandBuffer = *transientCB.m_pCommandBuffer;
    }

    /// @brief Add a signal semaphore operation. Value is ignored for binary semaphore
    void SignalSemaphore(vk::Semaphore& semaphore, uint64_t value = 0, vk::PipelineStageFlagBits2 stageMask = vk::PipelineStageFlagBits2::eNone)
    {
        auto& semaphoreSubmitInfo = m_signalSemaphoreSubmitInfos.emplace_back();
        semaphoreSubmitInfo.semaphore = semaphore;
        semaphoreSubmitInfo.stageMask = stageMask;
        semaphoreSubmitInfo.value = value;
    }

    /// @brief Add a wait semaphore operation. Value is ignored for binary semaphores
    void WaitSemaphore(const vk::Semaphore& semaphore, uint64_t value = 0, vk::PipelineStageFlagBits2 stageMask = vk::PipelineStageFlagBits2::eNone)
    {
        auto& semaphoreSubmitInfo = m_waitSemaphoreSubmitInfos.emplace_back();
        semaphoreSubmitInfo.semaphore = semaphore;
        semaphoreSubmitInfo.stageMask = stageMask;
        semaphoreSubmitInfo.value = value;
    }
};

class Queue
{
  private:
    vk::Queue m_queue;
    uint32_t m_familyIndex;

  public:
    Queue() {}

    Queue(vk::Device& pRenderEngine, uint32_t family)
    {
        m_queue = pRenderEngine.getQueue(family, 0);
        m_familyIndex = family;
    }

    void Submit(QueueSubmissionRequest& request, vk::Fence fence)
    {
        vk::SubmitInfo2 submitInfo = {
            .waitSemaphoreInfoCount = static_cast<uint32_t>(request.m_waitSemaphoreSubmitInfos.size()),
            .pWaitSemaphoreInfos = request.m_waitSemaphoreSubmitInfos.data(),
            .commandBufferInfoCount = static_cast<uint32_t>(request.m_commandBufferSubmitInfos.size()),
            .pCommandBufferInfos = request.m_commandBufferSubmitInfos.data(),
            .signalSemaphoreInfoCount = static_cast<uint32_t>(request.m_signalSemaphoreSubmitInfos.size()),
            .pSignalSemaphoreInfos = request.m_signalSemaphoreSubmitInfos.data(),
        };

        m_queue.submit2(submitInfo, fence);
    }

    void WaitIdle() { m_queue.waitIdle(); }

    const uint32_t GetFamilyIndex() const { return m_familyIndex; }
    vk::Queue& GetVkQueue() { return m_queue; }

    struct FamilyIndices
    {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;
        std::optional<uint32_t> transferFamily;

        bool IsComplete()
        {
            return graphicsFamily.has_value() && presentFamily.has_value() && transferFamily.has_value();
        }

        /// @brief Enumerate the queue families available on a physical device and index them.
        FamilyIndices(const vk::PhysicalDevice& physicalDevice, const vk::SurfaceKHR& surface)
        {
            assert(surface);

            // Assign index to queue families that could be found
            auto queueFamilies = physicalDevice.getQueueFamilyProperties();

            uint32_t i = 0;
            for (const auto& queueFamily : queueFamilies)
            {
                if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)
                {
                    graphicsFamily = i;
                }
                else if (queueFamily.queueFlags & vk::QueueFlagBits::eTransfer)
                {
                    transferFamily = i;
                }

                if (physicalDevice.getSurfaceSupportKHR(i, surface).value)
                {
                    presentFamily = i;
                }

                // TODO : It's very likely that the queue family that has the "present" capability is the same as
                //  the one that has "graphics". In the tutorial they are treated as if they were separate for a uniform approach.
                //  We can add logic to explicitly prefer m_physical devices that support both drawing and presentation in the same queue
                //  for improved performance.

                if (IsComplete())
                {
                    break;
                }
                i++;
            }
        }
    };
};
} // namespace aln