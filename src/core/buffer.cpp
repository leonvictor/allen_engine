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

        void destroy() override 
        {
            Allocation::destroy();
            device->logical.get().destroyBuffer(buffer);
        }

        void copyTo(vk::CommandBuffer& cb, vk::Image& image, std::vector<vk::BufferImageCopy> bufferCopyRegions)
        {
            cb.copyBufferToImage(this->buffer, image, vk::ImageLayout::eTransferDstOptimal, bufferCopyRegions);
        }

        void copyTo(vk::CommandBuffer& cb, vk::Image& image, const uint32_t width, const uint32_t height) const
        {
            vk::BufferImageCopy copy;
            copy.bufferOffset = 0;
            copy.bufferRowLength = 0;
            copy.bufferImageHeight = 0;
            copy.imageExtent = vk::Extent3D{width, height, 1};
            copy.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
            copy.imageSubresource.baseArrayLayer = 0;
            copy.imageSubresource.layerCount = 1;
            copy.imageSubresource.mipLevel = 0;
            copy.imageOffset = vk::Offset3D{0, 0, 0};

            cb.copyBufferToImage(this->buffer, image, vk::ImageLayout::eTransferDstOptimal, 1, &copy);
        }

        
        void copyTo(vk::CommandBuffer& cb, core::Buffer& dstBuffer, vk::DeviceSize size)
        {
            vk::BufferCopy copyRegion;
            copyRegion.srcOffset = 0;
            copyRegion.dstOffset = 0;
            copyRegion.size = size;

            cb.copyBuffer(this->buffer, dstBuffer.buffer, copyRegion);
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
        
            buffer = device->logical.get().createBuffer(bufferInfo);
        }

        void allocate(const vk::MemoryPropertyFlags& memProperties)
        {
            vk::MemoryRequirements memRequirements = device->logical.get().getBufferMemoryRequirements(buffer);
            Allocation::allocate(memRequirements, memProperties);
            device->logical.get().bindBufferMemory(buffer, memory, 0);
        }
    };
}