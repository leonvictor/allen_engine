#pragma once

#include <vulkan/vulkan.hpp>
#include "device.hpp"
#include "allocation.hpp"

#include <memory>

namespace core {
    class Buffer : public Allocation{
    public:
        vk::Buffer buffer;
        
        Buffer() {} // Empty ctor is required for now. Todo: Remove when we can 

        Buffer(std::shared_ptr<core::Device> device, vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags memProperties) {
            this->device = device;
            this->size = size;

            createBuffer(size, usage);
            allocate(memProperties);
        }

        void destroy() {
            Allocation::destroy();
            device->logicalDevice.destroyBuffer(buffer);
        }

        inline vk::DescriptorBufferInfo getDescriptor() {
            return vk::DescriptorBufferInfo(buffer, 0, size);
        }

        operator vk::Buffer() {
            return buffer;
        }

    private:
        void createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage) {
            // TODO: Move queues out of this function
            uint32_t queues[] = {device->queueFamilyIndices.graphicsFamily.value(), device->queueFamilyIndices.transferFamily.value()};
            
            vk::BufferCreateInfo bufferInfo;
            bufferInfo.size = size;
            bufferInfo.usage = usage;
            // TODO: We might have to pull this out as well
            bufferInfo.sharingMode = vk::SharingMode::eConcurrent; // Can buffers be shared between queues?
            bufferInfo.flags = vk::BufferCreateFlags(); // Configure sparse buffer memory. Not used rn
            
            bufferInfo.queueFamilyIndexCount = 2;
            bufferInfo.pQueueFamilyIndices = queues;
        
            buffer = device->logicalDevice.createBuffer(bufferInfo);
        }

        void allocate(const vk::MemoryPropertyFlags& memProperties)
        {
            vk::MemoryRequirements memRequirements = device->logicalDevice.getBufferMemoryRequirements(buffer);
            Allocation::allocate(memRequirements, memProperties);
            device->logicalDevice.bindBufferMemory(buffer, memory, 0);
        }
    };
}