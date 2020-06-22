#pragma once

#include "../utils/files.cpp"
#include "device.hpp"
#include <vulkan/vulkan.hpp>

namespace core::shaders
{
vk::ShaderModule createShaderModule(std::shared_ptr<core::Device> device, const std::string& filename);
vk::PipelineShaderStageCreateInfo loadShader(std::shared_ptr<core::Device> device, const std::string& filename, vk::ShaderStageFlagBits stage, const std::string& entryPoint);
} // namespace core::shaders