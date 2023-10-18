#pragma once

#include <filesystem>

#include <vulkan/vulkan.hpp>

namespace aln
{
class RenderEngine;

namespace shaders
{

struct ShaderInfo
{
    std::string entryPoint;
    vk::ShaderModule module;
    vk::ShaderStageFlagBits stage;

    vk::PipelineShaderStageCreateInfo GetCreateInfo()
    {
        vk::PipelineShaderStageCreateInfo createInfo = {
            .stage = stage,
            .module = module,
            .pName = entryPoint.c_str(),
        };
        return createInfo;
    }
};

ShaderInfo LoadShader(RenderEngine* pRenderEngine, const std::filesystem::path& shaderFilePath, const vk::ShaderStageFlagBits stage, std::string entryPoint);

} // namespace shaders
} // namespace aln