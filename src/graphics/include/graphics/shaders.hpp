#pragma once

#include "device.hpp"

#include <shaderc/shaderc.hpp>
#include <utils/files.hpp>

namespace aln::vkg::shaders
{
struct ShaderInfo
{
    std::string entryPoint;
    vk::ShaderModule module;
    vk::ShaderStageFlagBits stage;

    vk::PipelineShaderStageCreateInfo GetCreateInfo()
    {
        vk::PipelineShaderStageCreateInfo createInfo;
        createInfo.stage = stage;
        createInfo.module = module;
        createInfo.pName = entryPoint.c_str();
        return createInfo;
    }
};

vk::ShaderModule CreateShaderModule(std::shared_ptr<Device> device, const std::string& filename);
ShaderInfo LoadShader(std::shared_ptr<Device> device, const std::string& filename, const vk::ShaderStageFlagBits stage, std::string entryPoint);
std::vector<uint32_t> CompileGlslToSpvBinary(const std::string& source_name,
    shaderc_shader_kind kind,
    bool optimize = false);
} // namespace aln::vkg::shaders