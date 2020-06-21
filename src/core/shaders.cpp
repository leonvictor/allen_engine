#pragma once

#include <vulkan/vulkan.hpp>
#include "device.hpp"
#include "../utils/files.cpp"

namespace core::shaders {
    vk::ShaderModule createShaderModule(std::shared_ptr<core::Device> device, const std::string& filename) {
        auto code = utils::readFile(filename);
        return device->logicalDevice.get().createShaderModule({ {}, code.size(), reinterpret_cast<const uint32_t*>(code.data()) });
    }

    vk::PipelineShaderStageCreateInfo loadShader(std::shared_ptr<core::Device> device, const std::string& filename, vk::ShaderStageFlagBits stage, const std::string& entryPoint) {
        auto shaderModule = createShaderModule(device, filename);
        vk::PipelineShaderStageCreateInfo createInfo;
        createInfo.stage = stage; 
        createInfo.module = shaderModule;
        createInfo.pName = entryPoint.c_str();
        // pSpecializationInfo : We can set values for constants in the shader.
        // Then we can use a single shader module and have its behavior configured at pipeline creation (here)
        return createInfo;
    }
}