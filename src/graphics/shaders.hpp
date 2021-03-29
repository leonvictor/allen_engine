#pragma once

#include "../utils/files.cpp"
#include "device.hpp"
#include <shaderc/shaderc.hpp>

namespace vkg::shaders
{
vk::ShaderModule CreateShaderModule(std::shared_ptr<Device> device, const std::string& filename);
vk::PipelineShaderStageCreateInfo LoadShader(std::shared_ptr<Device> device, const std::string& filename, const vk::ShaderStageFlagBits stage, const std::string& entryPoint);
std::vector<uint32_t> CompileGlslToSpvBinary(const std::string& source_name,
                                             shaderc_shader_kind kind,
                                             bool optimize = false);
} // namespace vkg::shaders