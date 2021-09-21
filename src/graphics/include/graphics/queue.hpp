#pragma once

#include <optional>
#include <vulkan/vulkan.hpp>

namespace aln::vkg
{
class Queue
{
  private:
    vk::Queue m_vkQueue;
    uint32_t m_familyIndex;

  public:
    Queue() {}

    Queue(vk::Device& device, uint32_t family)
    {
        m_vkQueue = device.getQueue(family, 0);
        m_familyIndex = family;
    }

    void Submit(std::vector<vk::CommandBuffer> cbs)
    {
        vk::SubmitInfo submitInfo;
        submitInfo.commandBufferCount = (uint32_t) cbs.size();
        submitInfo.pCommandBuffers = cbs.data();

        // TODO: use fences
        m_vkQueue.submit(submitInfo, vk::Fence{});
    }

    void
    Submit(vk::SubmitInfo& submitInfo, vk::Fence& fence)
    {
        m_vkQueue.submit(submitInfo, fence);
    }

    void WaitIdle()
    {
        m_vkQueue.waitIdle();
    }

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
            // Assign index to queue families that could be found
            auto queueFamilies = physicalDevice.getQueueFamilyProperties();

            int i = 0;
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

                if (physicalDevice.getSurfaceSupportKHR(i, surface))
                {
                    presentFamily = i;
                }

                //TODO : It's very likely that the queue family that has the "present" capability is the same as
                // the one that has "graphics". In the tutorial they are treated as if they were separate for a uniform approach.
                // We can add logic to explicitly prefer m_physical devices that support both drawing and presentation in the same queue
                // for improved performance.

                if (IsComplete())
                {
                    break;
                }
                i++;
            }
        }
    };
};
} // namespace aln::vkg