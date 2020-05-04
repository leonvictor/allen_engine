#pragma once

#include <vulkan/vulkan.hpp>
#include "device.hpp"
#include "../utils/files.cpp"

namespace core::shaders {
    vk::UniqueShaderModule createShaderModule(std::shared_ptr<core::Device> device, const std::string& filename) {
        auto code = utils::readFile(filename);
        return device->logicalDevice.createShaderModuleUnique({ {}, code.size(), reinterpret_cast<const uint32_t*>(code.data()) });
    }

    vk::PipelineShaderStageCreateInfo loadShader(std::shared_ptr<core::Device> device, const std::string& filename, vk::ShaderStageFlagBits stage, const std::string& entryPoint) {
        vk::PipelineShaderStageCreateInfo createInfo;
        createInfo.stage = vk::ShaderStageFlagBits::eFragment; 
        auto shaderModule = createShaderModule(device, filename);
        createInfo.module = shaderModule.get();
        createInfo.pName = entryPoint.c_str();
        // pSpecializationInfo : We can set values for constants in the shader.
        // Then we can use a single shader module and have its behavior configured at pipeline creation (here)
        return createInfo;
    }
}