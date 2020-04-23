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
    private:
    };
};