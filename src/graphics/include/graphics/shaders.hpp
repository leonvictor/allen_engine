#pragma once

#include "device.hpp"

#include <shaderc/shaderc.hpp>
#include <utils/files.hpp>

namespace aln::vkg::shaders
{
vk::ShaderModule CreateShaderModule(std::shared_ptr<Device> device, const std::string& filename);
vk::PipelineShaderStageCreateInfo LoadShader(std::shared_ptr<Device> device, const std::string& filename, const vk::ShaderStageFlagBits stage, const std::string& entryPoint);
std::vector<uint32_t> CompileGlslToSpvBinary(const std::string& source_name,
    shaderc_shader_kind kind,
    bool optimize = false);
} // namespace aln::vkg::shaders