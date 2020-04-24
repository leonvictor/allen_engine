#pragma once

#include <vulkan/vulkan.hpp>
#include "device.hpp"

#include <memory>

namespace core {
    class Buffer {
    public:
        vk::Buffer buffer;
        vk::DeviceMemory memory;
        void *mapped{nullptr};

        std::shared_ptr<core::Device> device;
        
        Buffer() {} // Empty ctor is required for now. Todo: Remove when we can 

        Buffer(std::shared_ptr<core::Device> device, vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags memProperties) {
            this->device = device;

            createBuffer(size, usage);
            allocate(memProperties);
        }

        // ~Buffer() {
        //     if (mapped) {
        //         unmap();
        //     }
        //     if (memory) {
        //         device->logicalDevice.destroyBuffer(buffer);
        //         device->logicalDevice.freeMemory(memory);
        //     }
        // }
        void cleanup() {
            if (mapped) {
                unmap();
            }
            device->logicalDevice.destroyBuffer(buffer);
            device->logicalDevice.freeMemory(memory);
        }
        // vkDestroyBuffer(device->getCDevice(), *uniformBuffers[i], nullptr);
                // vkFreeMemory(device->getCDevice(), uniformBuffersMemory[i], nullptr);

        inline void* map(size_t offset, vk::DeviceSize size) {
            mapped = device->logicalDevice.mapMemory(memory, offset, size, vk::MemoryMapFlags());
            return mapped;
        }

        inline void unmap() {
            device->logicalDevice.unmapMemory(memory);
            mapped = nullptr;
        }

        // Examples also has an offset.
        inline void copy(const void* data, size_t size) {
            memcpy(mapped, data, size);
        }

        operator vk::Buffer() {
            return buffer;
        }

        /* Helper function to cast to C vulkan buffer. Remove when no longer necessary */
        operator VkBuffer() { return VkBuffer(buffer); }

    private:
        void createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage) {
            // TODO: Move queues out of this function
            uint32_t queues[] = {device->queueFamilyIndices.graphicsFamily.value(), device->queueFamilyIndices.transferFamily.value()};
            
            vk::BufferCreateInfo bufferInfo;
            bufferInfo.size = size; // Byte size of the buffer = vertex size
            bufferInfo.usage = usage; // This is a vertex buffer
            // TODO: We might have to pull this out as well
            bufferInfo.sharingMode = vk::SharingMode::eConcurrent; // Can buffers be shared between queues?
            bufferInfo.flags = vk::BufferCreateFlags(); // Configure sparse buffer memory. Not used rn
            
            bufferInfo.queueFamilyIndexCount = 2;
            bufferInfo.pQueueFamilyIndices = queues;
        
            buffer = device->logicalDevice.createBuffer(bufferInfo);  
        }

        void allocate(vk::MemoryPropertyFlags memProperties) {
            //TODO: Assert buffer created
            vk::MemoryRequirements memRequirements = device->logicalDevice.getBufferMemoryRequirements(buffer);

            vk::MemoryAllocateInfo allocInfo;
            allocInfo.allocationSize = memRequirements.size;
            allocInfo.memoryTypeIndex = device->findMemoryType(memRequirements.memoryTypeBits, memProperties);

            memory = device->logicalDevice.allocateMemory(allocInfo, nullptr);

            device->logicalDevice.bindBufferMemory(buffer, memory, 0);
        }
    };
}