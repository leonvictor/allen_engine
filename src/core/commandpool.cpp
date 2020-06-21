#pragma once

#include <vulkan/vulkan.hpp>
#include <memory>

#include "device.hpp"

namespace core {
    class CommandPool {
    public:
        vk::CommandPool pool;
        std::shared_ptr<core::Device> device;

        CommandPool() {} // TODO: We shouldn't need this
        
        CommandPool(std::shared_ptr<core::Device> device, uint32_t queueFamilyIndex, vk::CommandPoolCreateFlagBits flags = vk::CommandPoolCreateFlagBits()) {
            this->device = device;

            vk::CommandPoolCreateInfo createInfo(flags, queueFamilyIndex);
            pool = device->logicalDevice.createCommandPool(createInfo);
        }

        operator vk::CommandPool() { return pool; }
        
        //TODO: Delete when we're done transfering to c++
        operator VkCommandPool() { return VkCommandPool(pool); }

        std::vector<vk::CommandBuffer> beginSingleTimeCommands() {
            assert(pool && "You are trying to use an unallocated command pool.");

            // TODO: Replace w/ allocate fn
            vk::CommandBufferAllocateInfo allocInfo;
            allocInfo.level = vk::CommandBufferLevel::ePrimary;
            allocInfo.commandPool = pool;
            allocInfo.commandBufferCount = 1;

            auto commandBuffers = device->logicalDevice.allocateCommandBuffers(allocInfo);
            
            // Immediately start recording
            vk::CommandBufferBeginInfo beginInfo;
            beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

            for (int i = 0; i < commandBuffers.size(); i++) {
                commandBuffers[i].begin(beginInfo);
            }
            return commandBuffers;   
        }

        inline void execute(vk::Queue queue, const std::function<void (vk::CommandBuffer cb)> &func) {
            vk::CommandBufferAllocateInfo cbai{ this->pool, vk::CommandBufferLevel::ePrimary, 1};
            auto cbs = device->logicalDevice.allocateCommandBuffers(cbai);
            cbs[0].begin(vk::CommandBufferBeginInfo{});
            func(cbs[0]);
            cbs[0].end();
            vk::SubmitInfo submitInfo;
            submitInfo.commandBufferCount =  (uint32_t) cbs.size();
            submitInfo.pCommandBuffers = cbs.data();
            queue.submit(submitInfo, vk::Fence{});
            queue.waitIdle(); // TODO: Replace this by a fence so that we can schedule multiple transfers and wait for them all to complete
            device->logicalDevice.freeCommandBuffers(pool, cbs);
        }

        // TODO: This would make more sense in a Queue class, which can decide how to handle the commands.
        void endSingleTimeCommands(std::vector<vk::CommandBuffer> commandBuffers, vk::Queue& queue) {
            assert(pool && "You are trying to use an unallocated command pool.");

            for (int i = 0; i < commandBuffers.size(); i++) {
                commandBuffers[i].end();
            }

            vk::SubmitInfo submitInfo;
            submitInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());
            submitInfo.pCommandBuffers = commandBuffers.data();
        
            queue.submit(submitInfo, nullptr);
            queue.waitIdle(); // TODO: Replace this by a fence so that we can schedule multiple transfers and wait for them all to complete

            device->logicalDevice.freeCommandBuffers(pool, commandBuffers);
        }

        std::vector<vk::CommandBuffer> allocateCommandBuffers(int count, vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary) const {
            vk::CommandBufferAllocateInfo allocInfo;
            allocInfo.commandPool = pool;
            allocInfo.commandBufferCount = count;
            allocInfo.level = level; // Or secondary

            return device->logicalDevice.allocateCommandBuffers(allocInfo);
        }
    private:
    };
};