#pragma once

#include "command_buffer.hpp"

#include <common/containers/vector.hpp>

#include <vulkan/vulkan.hpp>

#include <optional>

namespace aln
{
class Queue
{
  public:
    // Wrapper around a SubmitInfo that automatically handles persistent CBs' events
    class SubmissionRequest
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

        void SignalSemaphores(std::span<vk::Semaphore> semaphores, uint64_t value = 0)
        {
            m_signalSemaphoreSubmitInfos.reserve(m_signalSemaphoreSubmitInfos.size() + semaphores.size());
            for (auto& semaphore : semaphores)
            {
                auto& semaphoreSubmitInfo = m_signalSemaphoreSubmitInfos.emplace_back();
                semaphoreSubmitInfo.semaphore = semaphore;
                semaphoreSubmitInfo.value = value;
            }
        }

        void SignalSemaphore(vk::Semaphore& semaphore, uint64_t value = 0, vk::PipelineStageFlagBits2 stageMask = vk::PipelineStageFlagBits2::eNone)
        {
            auto& semaphoreSubmitInfo = m_signalSemaphoreSubmitInfos.emplace_back();
            semaphoreSubmitInfo.semaphore = semaphore;
            semaphoreSubmitInfo.stageMask = stageMask;
            semaphoreSubmitInfo.value = value;
        }

        void WaitSemaphore(vk::Semaphore& semaphore, vk::PipelineStageFlagBits2 stageMask = vk::PipelineStageFlagBits2::eNone)
        {
            auto& semaphoreSubmitInfo = m_waitSemaphoreSubmitInfos.emplace_back();
            semaphoreSubmitInfo.semaphore = semaphore;
            semaphoreSubmitInfo.stageMask = stageMask;
        }
    };

  private:
    vk::Queue m_vkQueue;
    uint32_t m_familyIndex;

  public:
    Queue() {}

    Queue(vk::Device& pRenderEngine, uint32_t family)
    {
        m_vkQueue = pRenderEngine.getQueue(family, 0);
        m_familyIndex = family;
    }

    void Submit(SubmissionRequest& request, vk::Fence fence)
    {
        vk::SubmitInfo2 submitInfo = {
            .waitSemaphoreInfoCount = static_cast<uint32_t>(request.m_waitSemaphoreSubmitInfos.size()),
            .pWaitSemaphoreInfos = request.m_waitSemaphoreSubmitInfos.data(),
            .commandBufferInfoCount = static_cast<uint32_t>(request.m_commandBufferSubmitInfos.size()),
            .pCommandBufferInfos = request.m_commandBufferSubmitInfos.data(),
            .signalSemaphoreInfoCount = static_cast<uint32_t>(request.m_signalSemaphoreSubmitInfos.size()),
            .pSignalSemaphoreInfos = request.m_signalSemaphoreSubmitInfos.data(),
        };

        m_vkQueue.submit2(submitInfo, fence);
    }

    void WaitIdle() { m_vkQueue.waitIdle(); }

    const uint32_t GetFamilyIndex() const { return m_familyIndex; }
    vk::Queue& GetVkQueue() { return m_vkQueue; }

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