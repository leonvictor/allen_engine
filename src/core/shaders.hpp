#pragma once

#include "../utils/files.cpp"
#include "device.hpp"
#include <shaderc/shaderc.hpp>

namespace core::shaders
{
vk::ShaderModule createShaderModule(std::shared_ptr<core::Device> device, const std::string& filename);
vk::PipelineShaderStageCreateInfo loadShader(std::shared_ptr<core::Device> device, const std::string& filename, vk::ShaderStageFlagBits stage, const std::string& entryPoint);
std::vector<uint32_t> compileGlslToSpvBinary(const std::string& source_name,
                                             shaderc_shader_kind kind,
                                             bool optimize = false);
} // namespace core::shaders